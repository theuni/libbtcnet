// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_DIRECTCONN_H
#define LIBBTCNET_SRC_DIRECTCONN_H

#include "bareconn.h"
#include "connectionbase.h"
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
    void OnConnectFailure(short event, int error) final;

private:
    int m_retries;
};

#endif // LIBBTCNET_SRC_DIRECTCONN_H
