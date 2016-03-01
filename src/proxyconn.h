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
    void OnConnectSuccess() final;
    void OnConnectFailure(short type) final;
    void OnProxyFailure(int error) final;
    void OnProxySuccess(CConnection resolved) final;

private:
    int m_retries;
    event_type<bufferevent> m_bev;
};

#endif // BTCNET_PROXYCONN_H
