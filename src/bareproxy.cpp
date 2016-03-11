// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bareproxy.h"
#include "eventtypes.h"

#include "libbtcnet/connection.h"

#include <assert.h>
#include <string.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#endif

CBareProxy::CBareProxy(const CConnection& conn)
    : m_proxy_connection(conn)
{
}

CBareProxy::~CBareProxy() = default;

void CBareProxy::InitProxy(event_type<bufferevent>&& bev)
{
    assert(bev);
    assert(!m_bev);
    m_bev.swap(bev);
    static const uint8_t pchSocks5Init[] = {0x05, 0x01, 0x00};
    static const uint8_t pchSocks5InitAuth[] = {0x05, 0x02, 0x02, 0x00};

    bufferevent_setcb(m_bev, receive_init, nullptr, event_cb, this);
    bufferevent_setwatermark(m_bev, EV_READ, 2, 0);

    const CProxy& proxy = m_proxy_connection.GetProxy();
    assert(proxy.IsSet());
    const CProxyAuth& auth = proxy.GetAuth();
    if (auth.IsSet())
        bufferevent_write(m_bev, pchSocks5InitAuth, sizeof(pchSocks5InitAuth));
    else
        bufferevent_write(m_bev, pchSocks5Init, sizeof(pchSocks5Init));

    bufferevent_enable(m_bev, EV_READ | EV_WRITE);
}

void CBareProxy::ProxyFailure(int error)
{
    m_bev.free();
    OnProxyFailure(error);
}

bool CBareProxy::writeproto(bufferevent* bev, const CConnection& conn)
{
    bool resolve = conn.IsDNS() && conn.GetOptions().doResolve == CConnectionOptions::RESOLVE_ONLY;
    std::vector<uint8_t> vSocks5;
    vSocks5.push_back(0x05); // VER protocol version
    if (resolve) {
        vSocks5.push_back(0xf0); // CMD RESOLVE
    } else
        vSocks5.push_back(0x01); // CMD CONNECT
    vSocks5.push_back(0x00);     // RSV Reserved

    unsigned short port;

    if (conn.IsDNS()) {
        const std::string& host = conn.GetHost();
        port = conn.GetPort();
        if (conn.GetHost().size() > 255)
            return false;
        vSocks5.push_back(0x03); // ATYP DOMAINNAME
        vSocks5.push_back(host.size());
        vSocks5.insert(vSocks5.end(), host.begin(), host.end());
        vSocks5.push_back((port >> 8) & 0xFF);
        vSocks5.push_back((port >> 0) & 0xFF);
    } else {
        sockaddr_storage sock;
        memset(&sock, 0, sizeof(sock));
        int socksize = sizeof(sock);
        conn.GetSockAddr(reinterpret_cast<sockaddr*>(&sock), &socksize);
        if (sock.ss_family == AF_INET6) {
            assert((size_t)socksize >= sizeof(sockaddr_in6));
            in6_addr addr = reinterpret_cast<sockaddr_in6*>(&sock)->sin6_addr;
            port = reinterpret_cast<sockaddr_in6*>(&sock)->sin6_port;
            vSocks5.push_back(0x04); // ATYP IPV6
            vSocks5.insert(vSocks5.end(), addr.s6_addr, addr.s6_addr + sizeof(addr.s6_addr));
            vSocks5.insert(vSocks5.end(), reinterpret_cast<unsigned char*>(&port), reinterpret_cast<unsigned char*>(&port) + sizeof(port));
            assert(vSocks5.size() == 22);
        } else if (sock.ss_family == AF_INET) {
            assert((size_t)socksize >= sizeof(sockaddr_in));
            in_addr addr = reinterpret_cast<sockaddr_in*>(&sock)->sin_addr;
            port = reinterpret_cast<sockaddr_in*>(&sock)->sin_port;
            vSocks5.push_back(0x01); // ATYP IPV4
            vSocks5.insert(vSocks5.end(), reinterpret_cast<unsigned char*>(&addr), reinterpret_cast<unsigned char*>(&addr) + sizeof(addr));
            vSocks5.insert(vSocks5.end(), reinterpret_cast<unsigned char*>(&port), reinterpret_cast<unsigned char*>(&port) + sizeof(port));
            assert(vSocks5.size() == 10);
        } else
            return false;
    }
    bufferevent_setwatermark(bev, EV_READ, 8, 0);
    bufferevent_write(bev, &vSocks5[0], vSocks5.size());
    return true;
}

bool CBareProxy::write_auth(bufferevent* bev, const CProxyAuth& auth)
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
    bufferevent_write(bev, &vAuth[0], vAuth.size());
    return true;
}

void CBareProxy::read_final(bufferevent* bev, void* ctx)
{
    CBareProxy* data = static_cast<CBareProxy*>(ctx);
    const CConnection& conn = data->m_proxy_connection;
    evbuffer* input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    int result;
    assert(len >= 8);
    char pchRet2[5] = {};
    result = evbuffer_copyout(input, pchRet2, sizeof(pchRet2));
    assert(result == sizeof(pchRet2));
    (void)result;
    if (pchRet2[0] != 0x05 || pchRet2[1] != 0x00) {
        data->ProxyFailure(0);
        return;
    }
    size_t needSize = 6;
    size_t addrsize;
    int sockaddr_size;
    bool ip = true;
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    void* writeip = nullptr;
    switch (pchRet2[3]) {
    case 0x01:
        addrsize = 4;
        addr.ss_family = AF_INET;
        sockaddr_size = sizeof(sockaddr_in);
        reinterpret_cast<sockaddr_in*>(&addr)->sin_port = htons(conn.GetPort());
        writeip = &(reinterpret_cast<sockaddr_in*>(&addr)->sin_addr);
        break;
    case 0x04:
        addrsize = 16;
        addr.ss_family = AF_INET6;
        sockaddr_size = sizeof(sockaddr_in6);
        reinterpret_cast<sockaddr_in6*>(&addr)->sin6_port = htons(conn.GetPort());
        writeip = &(reinterpret_cast<sockaddr_in6*>(&addr)->sin6_addr);
        break;
    case 0x03:
        addrsize = pchRet2[4];
        needSize += 1;
        ip = false;
        sockaddr_size = 0;
        break;
    default:
        data->ProxyFailure(0);
        return;
    }

    needSize += addrsize;

    if (len < needSize) {
        bufferevent_setwatermark(bev, EV_READ, needSize, 0);
        return;
    }

    if (ip) {
        result = evbuffer_drain(input, 4);
        assert(result == 0);
        (void)result;
        result = evbuffer_remove(input, writeip, addrsize);
        assert(result == (int)addrsize);
        (void)result;
        // Throw this away.
        char port[2] = {};
        result = evbuffer_remove(input, port, sizeof(port));
        assert(result == sizeof(port));
        (void)result;
    } else {
        result = evbuffer_drain(input, 5 + addrsize + 2);
        assert(result == 0);
        (void)result;
    }

    CConnection ret = conn;
    if (ip && conn.IsDNS() && conn.GetOptions().doResolve == CConnectionOptions::RESOLVE_ONLY)
        ret = CConnection(conn.GetOptions(), conn.GetNetConfig(), conn.GetProxy(), reinterpret_cast<sockaddr*>(&addr), sockaddr_size);
    data->OnProxySuccess(std::move(data->m_bev), std::move(ret));
}

void CBareProxy::check_auth_response(bufferevent* bev, void* ctx)
{
    CBareProxy* data = static_cast<CBareProxy*>(ctx);
    const CConnection& conn(data->m_proxy_connection);
    char pchRetA[2];

    if (bufferevent_read(bev, pchRetA, sizeof(pchRetA)) != sizeof(pchRetA)) {
        data->ProxyFailure(0);
        return;
    }

    if (pchRetA[0] != 0x01 && pchRetA[1] != 0x00) {
        data->ProxyFailure(0);
        return;
    }

    bufferevent_setcb(bev, read_final, nullptr, event_cb, ctx);

    if (!writeproto(bev, conn)) {
        data->ProxyFailure(0);
        return;
    }
}

void CBareProxy::receive_init(bufferevent* bev, void* ctx)
{
    CBareProxy* data = static_cast<CBareProxy*>(ctx);
    const CConnection& conn(data->m_proxy_connection);
    const CProxyAuth& auth(conn.GetProxy().GetAuth());
    char pchRet1[2];
    if (bufferevent_read(bev, pchRet1, sizeof(pchRet1)) != sizeof(pchRet1)) {
        data->ProxyFailure(0);
        return;
    }
    if (pchRet1[0] != 0x05) {
        data->ProxyFailure(0);
        return;
    }

    bool useAuth = pchRet1[1] == 0x02;
    if (useAuth) {
        bufferevent_setcb(bev, check_auth_response, nullptr, event_cb, ctx);
        if (!write_auth(bev, auth)) {
            data->ProxyFailure(0);
            return;
        }
    } else {
        bufferevent_setcb(bev, read_final, nullptr, event_cb, ctx);
        if (!writeproto(bev, conn))
            data->ProxyFailure(0);
    }
}

void CBareProxy::event_cb(bufferevent* /*unused*/, short events, void* ctx)
{
    if (((events & BEV_EVENT_ERROR) != 0) || ((events & BEV_EVENT_EOF) != 0) || ((events & BEV_EVENT_TIMEOUT) != 0) || ((events & BEV_EVENT_CONNECTED) != 0)) {
        CBareProxy* data = static_cast<CBareProxy*>(ctx);
        data->ProxyFailure(0);
    }
}
