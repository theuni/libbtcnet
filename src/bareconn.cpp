// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bareconn.h"
#include "eventtypes.h"
#include <assert.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <functional>

CBareConnection::~CBareConnection() = default;

void CBareConnection::conn_event(bufferevent* bev, short event, void* ctx)
{
    assert(bev != nullptr);
    assert(ctx != nullptr);
    CBareConnection* data = static_cast<CBareConnection*>(ctx);
    if ((event & BEV_EVENT_CONNECTED) != 0)
        data->OnConnectSuccess(std::move(data->m_bev));
    else {
        evutil_socket_t sock = bufferevent_getfd(bev);
        (void)sock;
        // sock may be BAD_SOCKET here if it wasn't successfully created.
        int error = evutil_socket_geterror(sock);
        data->m_bev.free();
        data->OnConnectFailure(event, error);
    }
}

void CBareConnection::BareConnect(const event_type<event_base>& base, int bev_opts, evutil_socket_t socket, sockaddr* addr, int addrlen, const timeval& connTimeout)
{
    assert(!m_bev);
    assert(addr != nullptr);
    assert(addrlen > 0);

    m_bev = event_type<bufferevent>(bufferevent_socket_new(base, socket, bev_opts));

    int ret;
    ret = bufferevent_disable(m_bev, EV_READ | EV_WRITE);
    assert(ret == 0);
    (void)ret;
    ret = bufferevent_set_timeouts(m_bev, &connTimeout, &connTimeout);
    assert(ret == 0);
    (void)ret;
    bufferevent_setcb(m_bev, nullptr, nullptr, conn_event, this);
    bufferevent_socket_connect(m_bev, addr, addrlen);
}
