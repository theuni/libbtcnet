// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "directconn.h"
#include "logger.h"

#include <event2/bufferevent.h>
#include <event2/event.h>
#include <string.h>
#include <assert.h>

CDirectConnection::CDirectConnection(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id)
    : ConnectionBase(handler, std::move(conn), id), m_retries(m_connection.GetOptions().nRetries)
{
}

CDirectConnection::~CDirectConnection()
{
    Cancel();
}

bool CDirectConnection::IsOutgoing() const
{
    return true;
}

void CDirectConnection::Connect()
{
    assert(m_connection.IsSet());

    const CConnectionOptions& opts = m_connection.GetOptions();

    timeval connTimeout;
    connTimeout.tv_sec = opts.nConnTimeout;
    connTimeout.tv_usec = 0;

    sockaddr_storage addr_storage;
    memset(&addr_storage, 0, sizeof(addr_storage));
    int addrlen = sizeof(addr_storage);
    sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
    m_connection.GetSockAddr(addr, &addrlen);

    BareConnect(m_event_base, m_handler.GetBevOpts(), BAD_SOCKET, addr, addrlen, connTimeout);
}

void CDirectConnection::OnConnectSuccess(event_type<bufferevent>&& bev)
{
    m_retries = m_connection.GetOptions().nRetries;
    OnOutgoingConnected(std::move(bev), m_connection);
}

void CDirectConnection::OnConnectFailure(short event, int /*error*/)
{
    OnConnectionFailure(ConnectionFailureType::CONNECT, event, m_connection, m_retries > 0 ? m_retries-- != 0 : m_retries != 0);
}

void CDirectConnection::Cancel()
{
}
