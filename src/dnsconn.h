// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_DNSCONN_H
#define LIBBTCNET_SRC_DNSCONN_H

#include "bareconn.h"
#include "connectionbase.h"
#include "resolve.h"

class CConnection;
struct event_base;
struct bufferevent;

class CDNSConnection final : public ConnectionBase, public CBareConnection, public CDNSResolve
{
public:
    CDNSConnection(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id);
    ~CDNSConnection() final;
    void Connect() final;
    void Cancel() final;
    bool IsOutgoing() const final;

protected:
    void OnConnectSuccess(event_type<bufferevent>&& bev) final;
    void OnConnectFailure(short event, int error) final;
    void OnResolveSuccess(CDNSResponse&& response) final;
    void OnResolveFailure(int error) final;

private:
    void DoResolve();
    void ConnectResolved();
    int m_retries;
    CDNSResponse m_resolved;
    CDNSResponse::iterator m_iter;
    event_type<evdns_getaddrinfo_request> m_request;
    const event_type<evdns_base>& m_dns_base;
};

#endif // LIBBTCNET_SRC_DNSCONN_H
