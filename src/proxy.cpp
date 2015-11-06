// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proxy.h"
#include "callbacks.h"
#include "connection_data.h"
#include "libbtcnet/handler.h"
#include "libbtcnet/connection.h"

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <assert.h>
#include <vector>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#endif

struct proxy_callbacks
{
    static void receive_init(bufferevent *bev, void *ctx);
    static void check_auth_response(bufferevent *bev, void *ctx);
    static void read_final(bufferevent *bev, void *ctx);
    static void event_cb(bufferevent *bev, short events, void *ctx);
};

static bool writeproto(bufferevent *bev, const CConnection& conn);
static bool write_auth(bufferevent *bev, const CProxyAuth& auth);

void proxy_init(bufferevent *bev, const connection_data* data)
{
    static const uint8_t pchSocks5Init[] = { 0x05, 0x02, 0x00, 0x02, 0x05, 0x01, 0x00 };
    assert(data);
    const CProxy& proxy(data->conn.GetProxy());
    assert(proxy.IsSet());
    bufferevent_setwatermark(bev, EV_READ, 2, 0);
    bufferevent_setcb(bev, proxy_callbacks::receive_init, NULL, proxy_callbacks::event_cb, (void*)data);
    if (proxy.GetAuth().IsSet())
        bufferevent_write(bev,pchSocks5Init, 4);
    else
        bufferevent_write(bev,pchSocks5Init + 4, 3);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
}

bool writeproto(bufferevent *bev, const CConnection& conn)
{
    bool resolve = conn.IsDNS() && conn.GetOptions().doResolve == CConnectionOptions::RESOLVE_ONLY;
    std::vector<uint8_t> vSocks5;
    vSocks5.push_back(0x05);     // VER protocol version
    if(resolve)
    {
        vSocks5.push_back(0xf0); // CMD RESOLVE
    }
    else
        vSocks5.push_back(0x01); // CMD CONNECT
    vSocks5.push_back(0x00);     // RSV Reserved

    unsigned short port;

    if(conn.IsDNS())
    {
        const std::string& host = conn.GetHost();
        port = conn.GetPort();
        if(conn.GetHost().size() > 255)
            return false;
        vSocks5.push_back(0x03); // ATYP DOMAINNAME
        vSocks5.push_back(host.size());
        vSocks5.insert(vSocks5.end(), host.begin(), host.end());
        vSocks5.push_back((port >> 8) & 0xFF);
        vSocks5.push_back((port >> 0) & 0xFF);
    }
    else
    {
        sockaddr_storage sock;
        memset(&sock, 0, sizeof(sock));
        int socksize = sizeof(sock);
        conn.GetSockAddr((sockaddr*)&sock, &socksize);
        if (sock.ss_family == AF_INET6)
        {
            assert((size_t)socksize >= sizeof(sockaddr_in6));
            in6_addr addr = ((sockaddr_in6*)&sock)->sin6_addr;
            port = ((sockaddr_in6*)&sock)->sin6_port;
            vSocks5.push_back(0x04); // ATYP IPV6
            vSocks5.insert(vSocks5.end(), addr.s6_addr, addr.s6_addr + sizeof(addr.s6_addr));
            vSocks5.insert(vSocks5.end(), (unsigned char*)&port, (unsigned char*)&port + sizeof(port));
            assert(vSocks5.size() == 22);
        }
        else if(sock.ss_family == AF_INET)
        {
            assert((size_t)socksize >= sizeof(sockaddr_in));
            in_addr addr = ((sockaddr_in*)&sock)->sin_addr;
            port = ((sockaddr_in*)&sock)->sin_port;
            vSocks5.push_back(0x01); // ATYP IPV4
            vSocks5.insert(vSocks5.end(), (unsigned char*)&addr, (unsigned char*)&addr + sizeof(addr));
            vSocks5.insert(vSocks5.end(), (unsigned char*)&port, (unsigned char*)&port + sizeof(port));
            assert(vSocks5.size() == 10);
        }
        else
            return false;
    }
    bufferevent_setwatermark(bev, EV_READ, 8, 0);
    bufferevent_write(bev,&vSocks5[0], vSocks5.size());
    return true;
}

bool write_auth(bufferevent *bev, const CProxyAuth& auth)
{
    const std::string& username = auth.GetUsername();
    const std::string& password = auth.GetPassword();
    // Perform username/password authentication (as described in RFC1929)
    std::vector<uint8_t> vAuth;
    vAuth.push_back(0x01);
    if (username.size() > 255 || password.size() > 255)
        return false;
    vAuth.push_back(username.size());
    vAuth.insert(vAuth.end(), username.begin(), username.end());
    vAuth.push_back(password.size());
    vAuth.insert(vAuth.end(), password.begin(), password.end());
    bufferevent_setwatermark(bev, EV_READ, 2, 0);
    bufferevent_write(bev,&vAuth[0], vAuth.size());
    return true;
}

void proxy_callbacks::read_final(bufferevent *bev, void *ctx)
{
    connection_data* data = static_cast<connection_data*>(ctx);
    evbuffer* input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    if(len < 8)
    {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }
    char pchRet2[5] = {};
    assert(evbuffer_copyout(input, pchRet2, sizeof(pchRet2)) == sizeof(pchRet2));
    if (pchRet2[0] != 0x05 || pchRet2[1] != 0x00 ) {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }
    size_t needSize = 6;
    size_t addrsize;
    int sockaddr_size;
    bool ip = true;
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    void* writeip = NULL;
    switch (pchRet2[3])
    {
        case 0x01:
            addrsize = 4;
            addr.ss_family = AF_INET;
            sockaddr_size = sizeof(sockaddr_in);
            ((sockaddr_in*)&addr)->sin_port = htons(data->conn.GetPort());
            writeip = &((sockaddr_in*)&addr)->sin_addr;
            break;
        case 0x04:
            addrsize = 16;
            addr.ss_family = AF_INET6;
            sockaddr_size = sizeof(sockaddr_in6);
            ((sockaddr_in6*)&addr)->sin6_port = htons(data->conn.GetPort());
            writeip = &((sockaddr_in6*)&addr)->sin6_addr;
            break;
        case 0x03:
            addrsize = pchRet2[4];
            needSize += 1;
            ip = false;
            sockaddr_size = 0;
            break;
        default:
            data->handler->OutgoingConnectionFailure(data->id); return;
    }

    needSize += addrsize;

    if(len < needSize)
    {
        bufferevent_setwatermark(bev, EV_READ, needSize, 0);
        return;
    }

    if(ip)
    {
        assert(evbuffer_drain(input, 4) == 0);
        assert(evbuffer_remove(input, writeip, addrsize) == (int)addrsize);
        // Throw this away.
        char port[2] = {};
        assert(evbuffer_remove(input, port, sizeof(port)) == sizeof(port));
    }
    else
    {
        assert(evbuffer_drain(input, 5 + addrsize + 2) == 0);
    }
    if(data->conn.IsDNS() && data->conn.GetOptions().doResolve == CConnectionOptions::RESOLVE_ONLY)
    {
        if(ip)
        {
            CConnection resolved((sockaddr*)&addr, sockaddr_size, data->conn.GetOptions(), data->conn.GetNetConfig(), data->conn.GetProxy());
            std::list<CConnection> resolved_list(1, resolved);
            data->handler->LookupResults(data->id, resolved_list);
            return;
        }
    }
    else
    {
        data->handler->OutgoingConnected(data->id);
        return;
    }
    data->handler->OutgoingConnectionFailure(data->id);
}

void proxy_callbacks::check_auth_response(bufferevent *bev, void *ctx)
{
    connection_data* data = static_cast<connection_data*>(ctx);
    const CConnection& conn(data->conn);
    char pchRetA[2];

    if (bufferevent_read(bev, pchRetA, sizeof(pchRetA)) != sizeof(pchRetA))
    {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }

    if (pchRetA[0] != 0x01 && pchRetA[1] != 0x00)
    {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }

    bufferevent_setcb(bev, proxy_callbacks::read_final, NULL, proxy_callbacks::event_cb, ctx);

    if(!writeproto(bev, conn))
    {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }
}

void proxy_callbacks::receive_init(bufferevent *bev, void *ctx)
{
    connection_data* data =  static_cast<connection_data*>(ctx);
    const CConnection& conn(data->conn);
    const CProxyAuth& auth(conn.GetProxy().GetAuth());
    char pchRet1[2];
    if (bufferevent_read(bev, pchRet1, sizeof(pchRet1)) != sizeof(pchRet1))
    {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }
    if (pchRet1[0] != 0x05)
    {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }

    bool useAuth = pchRet1[1] == 0x02;
    if (useAuth)
    {
        bufferevent_setcb(bev, proxy_callbacks::check_auth_response, NULL, proxy_callbacks::event_cb, ctx);
        if(!write_auth(bev, auth))
        {
            data->handler->OutgoingConnectionFailure(data->id);
            return;
        }
    }
    else
    {
        bufferevent_setcb(bev, proxy_callbacks::read_final, NULL, proxy_callbacks::event_cb, ctx);
        if(!writeproto(bev, conn))
        {
            data->handler->OutgoingConnectionFailure(data->id);
            return;
        }
    }
}

void proxy_callbacks::event_cb(bufferevent *, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR || events & BEV_EVENT_EOF || events & BEV_EVENT_TIMEOUT || events & BEV_EVENT_CONNECTED)
    {
        connection_data* data = static_cast<connection_data*>(ctx);
        data->handler->OutgoingConnectionFailure(data->id);
    }
}
