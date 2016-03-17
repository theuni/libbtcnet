// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dnsconn.h"
#include "event2/bufferevent.h"
#include "event2/dns.h"

#if defined(_WIN32)
#include <ws2tcpip.h>
#endif

#include <string.h>
#include <assert.h>

CDNSConnection::CDNSConnection(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id)
    : ConnectionBase(handler, std::move(conn), id), m_retries(m_connection.GetOptions().nRetries), m_dns_base(handler.GetDNSBase())
{
}

bool CDNSConnection::IsOutgoing() const
{
    return true;
}

CDNSConnection::~CDNSConnection()
{
    Cancel();
}

void CDNSConnection::Connect()
{
    assert(!m_request);

    if (m_iter == m_resolved.end())
        DoResolve();
    else
        ConnectResolved();
}

void CDNSConnection::DoResolve()
{
    assert(!m_request);
    assert(m_resolved.empty());
    assert(m_iter == m_resolved.end());

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

    m_request = Resolve(m_dns_base, m_connection.GetHost().c_str(), std::to_string(m_connection.GetPort()).c_str(), &hint);
}

void CDNSConnection::OnResolveFailure(int error)
{
    assert(m_resolved.empty());
    assert(m_iter == m_resolved.end());

    m_request.reset(nullptr);
    OnConnectionFailure(ConnectionFailureType::RESOLVE, error, m_connection, m_retries > 0 ? m_retries-- != 0 : m_retries != 0);
}

void CDNSConnection::OnResolveSuccess(CDNSResponse&& response)
{
    assert(m_resolved.empty());
    assert(m_iter == m_resolved.end());

    m_request.reset(nullptr);
    m_resolved = std::move(response);
    m_iter = m_resolved.begin();
    ConnectResolved();
}

void CDNSConnection::OnConnectSuccess(event_type<bufferevent>&& bev)
{
    assert(bev);
    assert(!m_resolved.empty());
    assert(m_iter != m_resolved.end());
    assert(!m_request);

    CConnection resolved(m_connection.GetOptions(), m_connection.GetNetConfig(), m_iter->ai_addr, m_iter->ai_addrlen);
    m_resolved.clear();
    m_iter = m_resolved.end();
    m_retries = m_connection.GetOptions().nRetries;
    OnOutgoingConnected(std::move(bev), std::move(resolved));
}
void CDNSConnection::OnConnectFailure(short event, int /*error*/)
{
    assert(!m_request);
    assert(m_iter != m_resolved.end());
    assert(!m_resolved.empty());

    CConnection resolved(m_connection.GetOptions(), m_connection.GetNetConfig(), m_iter->ai_addr, m_iter->ai_addrlen);
    if (++m_iter == m_resolved.end())
        m_resolved.clear();
    OnConnectionFailure(ConnectionFailureType::CONNECT, event, std::move(resolved), m_iter != m_resolved.end() || m_retries != 0);
}

void CDNSConnection::ConnectResolved()
{
    assert(m_iter != m_resolved.end());
    assert(!m_resolved.empty());

    timeval connTimeout;
    connTimeout.tv_sec = m_connection.GetOptions().nInitialTimeout;
    connTimeout.tv_usec = 0;

    BareConnect(m_event_base, m_handler.GetBevOpts(), BAD_SOCKET, m_iter->ai_addr, m_iter->ai_addrlen, connTimeout);
}

void CDNSConnection::Cancel()
{
    m_request.free();
    m_resolved.clear();
    m_iter = m_resolved.end();
}
