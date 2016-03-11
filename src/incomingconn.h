// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_INCOMINGCONN_H
#define LIBBTCNET_SRC_INCOMINGCONN_H

#include "bareconn.h"
#include "connectionbase.h"

class CConnection;
struct evconnlistener;
struct bufferevent;

class CIncomingConn final : public ConnectionBase
{
public:
    CIncomingConn(CConnectionHandlerInt& handler, CConnection listen, ConnID id, evutil_socket_t sock, sockaddr* address, int socklen);
    ~CIncomingConn() final;
    void Connect() final;
    void Cancel() final;
    bool IsOutgoing() const final;

private:
    const evutil_socket_t m_sock;
    const CConnection m_incoming_conn;
    const int m_addrsize;
    sockaddr_storage m_addr;
};

#endif // LIBBTCNET_SRC_INCOMINGCONN_H
