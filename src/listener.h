// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_LISTENER_H
#define BTCNET_LISTENER_H

#include "connectionbase.h"
#include "bareconn.h"

class CConnection;
struct event_base;
struct evconnlistener;

class CConnListener
{
public:
    CConnListener(CConnectionHandlerInt& handler, const event_type<event_base>&, ConnID id, CConnection conn);
    ~CConnListener();
    bool Bind();
    void Unbind();
    bool Enable();
    bool Disable();
    const CConnection& GetListenConnection() const;

private:
    static void listen_error_cb(evconnlistener*, void* ctx);
    static void accept_conn(evconnlistener*, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx);
    CConnectionHandlerInt& m_handler;
    const event_type<event_base>& m_event_base;
    ConnID m_id;
    CConnection m_connection;
    event_type<evconnlistener> m_listener;
};

#endif // BTCNET_LISTENER_H
