// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "incomingconn.h"
#include "eventtypes.h"

#include <event2/bufferevent.h>
#include <event2/event.h>
#include <string.h>
#include <assert.h>

CIncomingConn::CIncomingConn(CConnectionHandlerInt& handler, CConnection listen, ConnID id, evutil_socket_t sock, sockaddr* address, int socklen)
    : ConnectionBase(handler, std::move(listen), id), m_sock(sock), m_addrsize(socklen)
{
    assert((size_t)socklen <= sizeof(m_addr));
    memset(&m_addr, 0, sizeof(m_addr));
    memcpy(&m_addr, address, socklen);
}

CIncomingConn::~CIncomingConn()
{
    Cancel();
}

bool CIncomingConn::IsOutgoing() const
{
    return false;
}

void CIncomingConn::Connect()
{
    event_type<bufferevent> bev(bufferevent_socket_new(m_event_base, m_sock, m_handler.GetBevOpts()));
    assert(bev);
    OnIncomingConnected(std::move(bev), reinterpret_cast<sockaddr*>(&m_addr), m_addrsize);
}

void CIncomingConn::Cancel()
{
}
