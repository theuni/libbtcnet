// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_INCOMINGCONN_H
#define BTCNET_INCOMINGCONN_H

#include "connectionbase.h"
#include "bareconn.h"

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

#endif // BTCNET_INCOMINGCONN_H
