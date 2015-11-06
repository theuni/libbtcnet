// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simpleconnman.h"

CSimpleConnManager::CSimpleConnManager(int max_outgoing)
: CConnectionHandler(false, max_outgoing, 0, 0, max_outgoing)
{}

bool CSimpleConnManager::OnShutdown()
{
    return true;
}

bool CSimpleConnManager::OnReceiveMessages(uint64_t id, CNodeMessages msgs)
{
    std::map<uint64_t, std::vector<unsigned char> > messages;
    if(msgs.empty())
        return true;
    m_messages[id].splice(m_messages[id].end(), msgs, msgs.begin(), msgs.end());
    while(!m_messages.empty())
    {
        for(std::map<uint64_t, CNodeMessages>::iterator it = m_messages.begin(); it != m_messages.end();)
        {
            messages[it->first] = it->second.front();
            if(it->second.size() == 1)
                m_messages.erase(it++);
            else
            {
                it->second.pop_front();
                ++it;
            }
        }
        for(std::map<uint64_t, std::vector<unsigned char> >::iterator it = messages.begin(); it != messages.end(); ++it)
        {
            CSimpleNode* node = m_nodemanager.GetNode(it->first);
            if(node)
            {
                ProcessMessage(node, it->second);
                m_nodemanager.Unref(it->first);
            }
            else
                m_messages.erase(it->first);
        }
        messages.clear();
    }
    return true;
}

void CSimpleConnManager::AddSeed(const CConnection& conn)
{
    m_addrmanager.AddSeed(conn);
}

void CSimpleConnManager::AddAddress(const CConnection& conn)
{
    m_addrmanager.Add(conn);
}

CConnection CSimpleConnManager::GetAddress()
{
    return m_addrmanager.Get();
}

void CSimpleConnManager::OnWriteBufferFull(uint64_t id, size_t)
{
    PauseRecv(id);
}

void CSimpleConnManager::OnWriteBufferReady(uint64_t id, size_t)
{
    UnpauseRecv(id);
}

void CSimpleConnManager::OnDnsFailure(const CConnection&)
{
}

void CSimpleConnManager::OnConnectionFailure(const CConnection&, const CConnection&, uint64_t, bool, uint64_t)
{
}

bool CSimpleConnManager::OnIncomingConnection(const CConnectionListener&, const CConnection&, uint64_t)
{
    return false;
}

bool CSimpleConnManager::OnOutgoingConnection(const CConnection& conn, const CConnection&, uint64_t, uint64_t id)
{
    m_nodemanager.AddNode(id, CSimpleNode(id, conn.GetNetConfig(), false));
    return true;
}

void CSimpleConnManager::OnBindFailure(const CConnectionListener&, uint64_t, bool, uint64_t)
{
}

void CSimpleConnManager::OnBind(const CConnectionListener&, uint64_t, uint64_t)
{
}

bool CSimpleConnManager::OnNeedIncomingListener(CConnectionListener&, uint64_t)
{
    return false;
}

void CSimpleConnManager::OnDisconnected(const CConnection&, uint64_t id, bool, uint64_t)
{
    m_nodemanager.Remove(id);
}

void CSimpleConnManager::OnMalformedMessage(uint64_t id)
{
    CloseConnection(id, true);
}
