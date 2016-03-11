// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proxyconn.h"

#include <string.h>
#include <assert.h>

CProxyConn::CProxyConn(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id)
    : ConnectionBase(handler, std::move(conn), id), CBareProxy(m_connection), m_retries(m_connection.GetOptions().nRetries)
{
}

CProxyConn::~CProxyConn()
{
    CProxyConn::Cancel();
}

bool CProxyConn::IsOutgoing() const
{
    return true;
}

void CProxyConn::Connect()
{
    assert(m_connection.IsSet());

    const CConnectionOptions& opts = m_connection.GetOptions();

    timeval connTimeout;
    connTimeout.tv_sec = opts.nInitialTimeout;
    connTimeout.tv_usec = 0;

    const CConnectionBase& proxy = m_connection.GetProxy();
    assert(proxy.IsSet());

    sockaddr_storage addr_storage;
    int addrlen = sizeof(addr_storage);
    memset(&addr_storage, 0, sizeof(addr_storage));
    sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
    proxy.GetSockAddr(addr, &addrlen);

    BareConnect(m_event_base, m_handler.GetBevOpts(), BAD_SOCKET, addr, addrlen, connTimeout);
}

void CProxyConn::OnConnectSuccess(event_type<bufferevent>&& bev)
{
    InitProxy(std::move(bev));
}

void CProxyConn::OnConnectFailure(short event, int /*error*/)
{
    OnConnectionFailure(ConnectionFailureType::PROXY, event, m_connection, m_retries > 0 ? m_retries-- != 0 : m_retries != 0);
}

void CProxyConn::OnProxyFailure(int event)
{
    OnConnectionFailure(ConnectionFailureType::CONNECT, event, m_connection, m_retries > 0 ? m_retries-- != 0 : m_retries != 0);
}

void CProxyConn::OnProxySuccess(event_type<bufferevent>&& bev, CConnection resolved)
{
    OnOutgoingConnected(std::move(bev), std::move(resolved));
}

void CProxyConn::Cancel()
{
}
