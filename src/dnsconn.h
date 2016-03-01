// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_DNSCONN_H
#define BTCNET_DNSCONN_H

#include "connectionbase.h"
#include "bareconn.h"
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
    void OnConnectSuccess() final;
    void OnConnectFailure(short type) final;
    void OnResolveSuccess(CDNSResponse&& response) final;
    void OnResolveFailure(int result) final;

private:
    void DoResolve();
    void ConnectResolved();
    void ConnectionFailure(short event);
    int m_retries;
    CDNSResponse m_resolved;
    CDNSResponse::iterator m_iter;
    event_type<evdns_getaddrinfo_request> m_request;
    event_type<bufferevent> m_bev;
    const event_type<evdns_base>& m_dns_base;
};

#endif // BTCNET_DNSCONN_H
