// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_PROXYCONN_H
#define LIBBTCNET_SRC_PROXYCONN_H

#include "bareconn.h"
#include "bareproxy.h"
#include "connectionbase.h"

class CConnection;
struct event_base;
struct bufferevent;

class CProxyConn final : public ConnectionBase, public CBareConnection, public CBareProxy
{
public:
    CProxyConn(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id);
    ~CProxyConn() final;
    void Connect() final;
    void Cancel() final;
    bool IsOutgoing() const final;

protected:
    void OnConnectSuccess(event_type<bufferevent>&& bev) final;
    void OnConnectFailure(short event, int error) final;
    void OnProxyFailure(int event) final;
    void OnProxySuccess(event_type<bufferevent>&& bev, CConnection resolved) final;

private:
    int m_retries;
};

#endif // LIBBTCNET_SRC_PROXYCONN_H
