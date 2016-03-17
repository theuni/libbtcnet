// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "resolveonly.h"
#include "event2/bufferevent.h"
#include "event2/dns.h"
#include <string.h>
#include <assert.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#endif

CResolveOnly::CResolveOnly(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id)
    : m_id(id), m_connection(std::move(conn)), m_retries(m_connection.GetOptions().nRetries), m_handler(handler), m_retry_event(handler.GetEventBase(), 0, std::bind(&CResolveOnly::Resolve, this)), m_retry_timeout({m_connection.GetOptions().nRetryInterval, 0})
{
}

void CResolveOnly::Resolve()
{
    assert(!m_request);
    assert(m_connection.IsDNS());

    const CConnectionOptions& opts = m_connection.GetOptions();
    evutil_addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_flags = EVUTIL_AI_NUMERICSERV;
    hint.ai_flags |= opts.doResolve == CConnectionOptions::NO_RESOLVE ? EVUTIL_AI_NUMERICHOST : EVUTIL_AI_ADDRCONFIG;

    if (opts.resolveFamily == CConnectionOptions::IPV4)
        hint.ai_family = AF_INET;
    else if (opts.resolveFamily == CConnectionOptions::IPV6)
        hint.ai_family = AF_INET6;
    else
        hint.ai_family = AF_UNSPEC;
    m_request = CDNSResolve::Resolve(m_handler.GetDNSBase(), m_connection.GetHost().c_str(), std::to_string(m_connection.GetPort()).c_str(), &hint);
}

void CResolveOnly::OnResolveFailure(int error)
{
    m_request.reset(nullptr);
    m_handler.OnResolveFailure(m_id, m_connection, error, m_retries > 0 ? m_retries-- != 0 : m_retries != 0);
}

void CResolveOnly::OnResolveSuccess(CDNSResponse&& response)
{
    m_request.reset(nullptr);
    std::list<CConnection> connections;
    for (auto it = response.begin(); it != response.end(); ++it)
        connections.emplace_back(m_connection.GetOptions(), m_connection.GetNetConfig(), it->ai_addr, it->ai_addrlen);
    m_retries = m_connection.GetOptions().nRetries;
    m_handler.OnResolveComplete(m_id, m_connection, std::move(connections));
}

void CResolveOnly::Retry()
{
    m_retry_event.active();
}
