// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_PROXYCONN_H
#define BTCNET_PROXYCONN_H

#include "connectionbase.h"
#include "bareconn.h"
#include "bareproxy.h"

class CConnection;
struct event_base;
struct bufferevent;

class CProxyConn : public ConnectionBase, public CBareConnection, public CBareProxy
{
public:
    CProxyConn(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id);
    ~CProxyConn() final;
    void Connect() final;
    void Cancel() final;
    bool IsOutgoing() const final;

protected:
    void OnConnectSuccess(event_type<bufferevent>&& bev) final;
    void OnConnectFailure(short type, int error) final;
    void OnProxyFailure(int error) final;
    void OnProxySuccess(event_type<bufferevent>&& bev, CConnection resolved) final;

private:
    int m_retries;
};

#endif // BTCNET_PROXYCONN_H
