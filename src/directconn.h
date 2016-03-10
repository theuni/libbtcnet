// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_DIRECTCONN_H
#define BTCNET_DIRECTCONN_H

#include "connectionbase.h"
#include "bareconn.h"
#include "eventtypes.h"

class CConnection;
struct event_base;
struct bufferevent;

class CDirectConnection final : public ConnectionBase, public CBareConnection
{
public:
    CDirectConnection(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id);
    ~CDirectConnection() final;
    void Connect() final;
    void Cancel() final;
    bool IsOutgoing() const final;

protected:
    void OnConnectSuccess(event_type<bufferevent>&& bev) final;
    void OnConnectFailure(short type, int error) final;

private:
    int m_retries;
};

#endif // BTCNET_DIRECTCONN_H
