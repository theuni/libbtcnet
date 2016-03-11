// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_BARECONN_H
#define LIBBTCNET_SRC_BARECONN_H

#include "eventtypes.h"

#include <event2/util.h>

struct event_base;
struct bufferevent;
struct timeval;

static constexpr evutil_socket_t BAD_SOCKET = -1;

class CBareConnection
{
protected:
    CBareConnection() = default;
    virtual ~CBareConnection();
    virtual void OnConnectSuccess(event_type<bufferevent>&& bev) = 0;
    virtual void OnConnectFailure(short type, int err) = 0;
    void BareConnect(const event_type<event_base>& base, int bev_opts, evutil_socket_t socket, sockaddr* addr, int addrlen, const timeval& connTimeout);

private:
    static void conn_event(bufferevent* bev, short event, void* ctx);
    event_type<bufferevent> m_bev;
};

#endif // LIBBTCNET_SRC_BARECONN_H
