// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simpleconnman.h"
#include <stdio.h>
CSimpleConnManager::CSimpleConnManager(size_t max_queue_size)
: CConnectionHandler(false), m_stop(false), m_max_queue_size(max_queue_size)
{
}

void CSimpleConnManager::Stop()
{
    m_stop = true;
}

void CSimpleConnManager::Start(int outgoing_limit, int bind_limit)
{
    std::map<uint64_t, std::vector<unsigned char> > messages;
    std::map<uint64_t, std::vector<unsigned char> > first_messages;
    m_stop = false;
    CConnectionHandler::Start(outgoing_limit, bind_limit);
    while(!m_stop)
    {
        PumpEvents(m_messages.empty() && m_ready_for_first_send.empty());
        if(m_stop)
            break;
        for(std::map<uint64_t, CNodeMessages>::iterator it = m_messages.begin(); it != m_messages.end();)
        {
            if(it->second.first_receive)
            {
                first_messages[it->first] = it->second.messages.front();
                it->second.first_receive = false;
            }
            else
                messages[it->first] = it->second.messages.front();

            size_t msgsize = it->second.messages.front().size();

            if(it->second.size >= m_max_queue_size && it->second.size - msgsize < m_max_queue_size)
                UnpauseRecv(it->first);

            it->second.size -= msgsize;

            if(it->second.messages.size() == 1)
            {
                assert(it->second.size == 0);
                m_messages.erase(it++);
            }
            else
            {
                it->second.messages.pop_front();
                ++it;
            }
        }

        PumpEvents(false);
        if(m_stop)
            break;

        if(!m_ready_for_first_send.empty())
        {
            for(std::set<uint64_t>::const_iterator it = m_ready_for_first_send.begin(); it != m_ready_for_first_send.end(); ++it)
            {
                CSimpleNode* node = m_nodemanager.GetNode(*it);
                if(node)
                {
                    OnReadyForFirstSend(node);
                    m_nodemanager.Unref(*it);
                }
                if(!m_stop)
                    PumpEvents(false);
                if(m_stop)
                    break;
            }
            m_ready_for_first_send.clear();
        }

        if(m_stop)
            break;

        if(!first_messages.empty())
        {
            for(std::map<uint64_t, std::vector<unsigned char> >::iterator it = first_messages.begin(); it != first_messages.end(); ++it)
            {
                CSimpleNode* node = m_nodemanager.GetNode(it->first);
                if(node)
                {
                    ProcessFirstMessage(node, it->second);
                    m_nodemanager.Unref(it->first);
                }
                if(!m_stop)
                    PumpEvents(false);
                if(m_stop)
                    break;
            }
            first_messages.clear();
        }

        if(m_stop)
            break;

        if(!messages.empty())
        {
            for(std::map<uint64_t, std::vector<unsigned char> >::iterator it = messages.begin(); it != messages.end(); ++it)
            {
                CSimpleNode* node = m_nodemanager.GetNode(it->first);
                if(node)
                {
                    ProcessMessage(node, it->second);
                    m_nodemanager.Unref(it->first);
                }
                if(!m_stop)
                    PumpEvents(false);
                if(m_stop)
                    break;
            }
            messages.clear();
        }
    }

    CConnectionHandler::Shutdown();
    m_messages.clear();
    m_ready_for_first_send.clear();
}

bool CSimpleConnManager::OnReceiveMessages(uint64_t id, CNodeMessages& msgs)
{
    if(!msgs.size)
        return true;
    if(m_messages.find(id) == m_messages.end())
    {
        m_messages.insert(std::make_pair(id, msgs));
    }
    else
    {
        m_messages[id].messages.splice(m_messages[id].messages.end(), msgs.messages, msgs.messages.begin(), msgs.messages.end());
        m_messages[id].size += msgs.size;
    }

    if(m_messages[id].size < m_max_queue_size && m_messages[id].size + msgs.size >= m_max_queue_size)
        PauseRecv(id);

    return true;
}

void CSimpleConnManager::AddSeed(const CConnection& conn)
{
    m_addrmanager.AddSeed(conn);
}

void CSimpleConnManager::AddListener(const CConnectionListener& conn)
{
    m_listeners.push_back(conn);
}

void CSimpleConnManager::AddAddress(const CConnection& conn)
{
    m_addrmanager.Add(conn);
}

bool CSimpleConnManager::GetAddress(CConnection& conn)
{
    return m_addrmanager.Get(conn);
}

void CSimpleConnManager::OnWriteBufferFull(uint64_t id, size_t)
{
    PauseRecv(id);
}

void CSimpleConnManager::OnWriteBufferReady(uint64_t id, size_t)
{
    UnpauseRecv(id);
}

void CSimpleConnManager::OnReadyForFirstSend(const CConnection& conn, uint64_t id)
{
    m_ready_for_first_send.insert(id);
}

void CSimpleConnManager::OnDnsFailure(const CConnection&)
{
}

void CSimpleConnManager::OnConnectionFailure(const CConnection&, const CConnection&, uint64_t id, bool, uint64_t)
{
}

bool CSimpleConnManager::OnIncomingConnection(const CConnectionListener&, const CConnection& conn, uint64_t id, int incount, int totalincount)
{
    m_nodemanager.AddNode(id, CSimpleNode(id, conn, true));
    return true;
}

bool CSimpleConnManager::OnOutgoingConnection(const CConnection& conn, const CConnection&, uint64_t, uint64_t id)
{
    m_nodemanager.AddNode(id, CSimpleNode(id, conn, false));
    return true;
}

void CSimpleConnManager::OnBindFailure(const CConnectionListener&, uint64_t, bool, uint64_t)
{
}

void CSimpleConnManager::OnBind(const CConnectionListener&, uint64_t, uint64_t)
{
}

bool CSimpleConnManager::OnNeedIncomingListener(CConnectionListener& listener, uint64_t)
{
    if(!m_listeners.empty())
    {
        listener = m_listeners.front();
        m_listeners.pop_front();
        return true;
    }
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

void CSimpleConnManager::OnProxyConnected(uint64_t id, const CConnection& conn)
{
}

void CSimpleConnManager::OnProxyFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid)
{
}

void CSimpleConnManager::OnBytesRead(uint64_t id, size_t bytes, size_t total_bytes)
{
}

void CSimpleConnManager::OnBytesWritten(uint64_t id, size_t bytes, size_t total_bytes)
{
}
