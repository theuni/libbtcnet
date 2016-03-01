// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bareconn.h"
#include "eventtypes.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <assert.h>
#include <functional>

CBareConnection::CBareConnection()
{
}

CBareConnection::~CBareConnection()
{
}

void CBareConnection::conn_event(bufferevent* bev, short event, void* ctx)
{
    assert(bev != nullptr);
    assert(ctx != nullptr);
    CBareConnection* data = static_cast<CBareConnection*>(ctx);
    bufferevent_setcb(bev, nullptr, nullptr, nullptr, nullptr);
    if (event & BEV_EVENT_CONNECTED)
        data->OnConnectSuccess();
    else {
        data->OnConnectFailure(event);
    }
}

event_type<bufferevent> CBareConnection::BareCreate(const event_type<event_base>& base, evutil_socket_t socket, int bev_opts)
{
    return event_type<bufferevent>(bufferevent_socket_new(base, socket, bev_opts));
}

bool CBareConnection::BareConnect(const event_type<bufferevent>& bev, sockaddr* addr, int addrlen, const timeval& connTimeout)
{
    assert(bev != nullptr);
    assert(addr != nullptr);
    assert(addrlen > 0);

    int ret;
    ret = bufferevent_disable(bev, EV_READ | EV_WRITE);
    assert(ret == 0);
    ret = bufferevent_set_timeouts(bev, &connTimeout, &connTimeout);
    assert(ret == 0);
    bufferevent_setcb(bev, nullptr, nullptr, conn_event, this);
    return bufferevent_socket_connect(bev, addr, addrlen) == 0;
}
