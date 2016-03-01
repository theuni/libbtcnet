// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_BARECONN_H
#define BTCNET_BARECONN_H

#include <event2/util.h>

template <typename T>
class event_type;

struct event_base;
struct bufferevent;
struct timeval;

static constexpr evutil_socket_t BAD_SOCKET = -1;

class CBareConnection
{
protected:
    CBareConnection();
    virtual ~CBareConnection();
    virtual void OnConnectSuccess() = 0;
    virtual void OnConnectFailure(short type) = 0;
    bool BareConnect(const event_type<bufferevent>& bev, sockaddr* addr, int addrlen, const timeval&);
    static event_type<bufferevent> BareCreate(const event_type<event_base>& base, evutil_socket_t socket, int bev_opts);

private:
    static void conn_event(bufferevent* bev, short event, void* ctx);
};

#endif // BTCNET_BARECONN_H
