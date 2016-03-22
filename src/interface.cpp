// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libbtcnet/handler.h"
#include "libbtcnet/connection.h"
#include "handler.h"

CConnectionHandler::CConnectionHandler(bool enable_threading)
{
    m_internal = new CConnectionHandlerInt(*this, enable_threading);
}

CConnectionHandler::~CConnectionHandler()
{
    delete m_internal;
}

void CConnectionHandler::SetIncomingRateLimit(const CRateLimit& limit)
{
    m_internal->SetIncomingRateLimit(limit);
    ;
}

void CConnectionHandler::SetOutgoingRateLimit(const CRateLimit& limit)
{
    m_internal->SetOutgoingRateLimit(limit);
}

void CConnectionHandler::CloseConnection(ConnID id, bool immediately)
{
    m_internal->CloseConnection(id, immediately);
}

void CConnectionHandler::SetRateLimit(ConnID id, const CRateLimit& limit)
{
    m_internal->SetRateLimit(id, limit);
}

void CConnectionHandler::PauseRecv(ConnID id)
{
    m_internal->PauseRecv(id);
}

void CConnectionHandler::UnpauseRecv(ConnID id)
{
    m_internal->UnpauseRecv(id);
}

void CConnectionHandler::Shutdown()
{
    m_internal->Shutdown();
}

bool CConnectionHandler::Bind(const CConnection& conn)
{
    return m_internal->Bind(conn);
}

bool CConnectionHandler::PumpEvents(bool block)
{
    return m_internal->PumpEvents(block);
}

void CConnectionHandler::Start(int outgoing_limit)
{
    m_internal->Start(outgoing_limit);
}

bool CConnectionHandler::Send(ConnID id, const unsigned char* data, size_t size)
{
    return m_internal->Send(id, data, size);
}

void CConnectionHandler::ResetPingTimeout(ConnID id, int seconds)
{
    m_internal->ResetPingTimeout(id, seconds);
}
