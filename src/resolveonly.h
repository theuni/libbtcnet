// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_RESOLVEONLY_H
#define BTCNET_RESOLVEONLY_H

#include "resolve.h"
#include "handler.h"
#include "libbtcnet/connection.h"

class CConnection;
class CConnectionHandlerInt;

struct event_base;

class CResolveOnly final : public CDNSResolve
{
public:
    CResolveOnly(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id);
    ~CResolveOnly() final = default;
    void Resolve();
    void Retry();

protected:
    void OnResolveFailure(int result) final;
    void OnResolveSuccess(CDNSResponse&& response) final;

private:
    const ConnID m_id;
    const CConnection m_connection;
    int m_retries;
    event_type<evdns_getaddrinfo_request> m_request;
    CConnectionHandlerInt& m_handler;
    CEvent m_retry_event;
    const timeval m_retry_timeout;
};

#endif // BTCNET_RESOLVEONLY_H
