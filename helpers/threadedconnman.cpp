// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "threadedconnman.h"
#include "threadednode.h"
//#include "net/threading.h"

#include <stdio.h>
#include <set>
#include <list>

CThreadedConnManager::CThreadedConnManager(int max_outgoing)
: CConnectionHandler(true, max_outgoing, 0, 0, max_outgoing), m_stop(false)
{}

bool CThreadedConnManager::OnShutdown()
{
    {
        //lock_guard_type lock(m_mutex);
        lock_guard_type lock(m_mutex);
        m_stop = true;
    }
    m_condvar.notify_one();
    return true;
}

void CThreadedConnManager::Run()
{
    std::map<uint64_t, std::vector<unsigned char> > messages;
    while(true)
    {
        {
            unique_lock_type lock(m_mutex);
            while(!m_stop && m_message_queue.empty())
                m_condvar.wait(lock);
            if(m_stop)
                break;
            for(std::map<uint64_t, CNodeMessages>::iterator i = m_message_queue.begin(); i != m_message_queue.end();)
            {
                uint64_t id = i->first;
                if(!i->second.empty())
                {
                    messages[id] = i->second.front();
                    if(i->second.size() == 1)
                    {
                        m_message_queue.erase(i++);
                    }
                    else
                    {
                        i->second.pop_front();
                        ++i;
                    }
                }
/*
                if(m_paused.count(id) && m_message_queue[id].totalsize() < 5000000)
                {
                    printf("resuming recv for: %lu\n", id);
                    m_paused.erase(id);
                    m_handler.UnpauseRecv(id);
                }
*/
            }
        }
        for(std::map<uint64_t, std::vector<unsigned char> >::iterator i = messages.begin(); i != messages.end();++i)
        {
            printf("processing message\n");
            uint64_t id = i->first;
            CThreadedNode* node = m_nodemanager.GetNode(id);
            if(node)
            {
                ProcessMessage(node, i->second);
                m_nodemanager.Release(id);
            }
        }
        messages.clear();
    }
}

bool CThreadedConnManager::OnReceiveMessages(uint64_t id, CNodeMessages msgs)
{
    {
        lock_guard_type lock(m_mutex);
        m_message_queue[id].splice(m_message_queue[id].end(), msgs, msgs.begin(), msgs.end());
    }
/*
    if(!m_paused.count(id) && m_message_queue[id].totalsize() >= 5000000)
    {
        printf("pausing recv for: %lu\n", id);
        m_paused.insert(id);
        PauseRecv(id);
    }
*/
    m_condvar.notify_one();
    return true;
}

void CThreadedConnManager::AddSeed(const CConnection& conn)
{
    m_addrmanager.AddSeed(conn);
}

void CThreadedConnManager::AddAddress(const CConnection& conn)
{
    m_addrmanager.Add(conn);
}

CConnection CThreadedConnManager::GetAddress()
{
    return m_addrmanager.Get();
}

void CThreadedConnManager::OnWriteBufferFull(uint64_t id, size_t)
{
    PauseRecv(id);
}

void CThreadedConnManager::OnWriteBufferReady(uint64_t id, size_t)
{
    UnpauseRecv(id);
}

void CThreadedConnManager::OnDnsFailure(const CConnection&)
{
}

void CThreadedConnManager::OnConnectionFailure(const CConnection&, const CConnection&, uint64_t, bool, uint64_t)
{
}

bool CThreadedConnManager::OnIncomingConnection(const CConnectionListener&, const CConnection&, uint64_t)
{
    return false;
}

bool CThreadedConnManager::OnOutgoingConnection(const CConnection& conn, const CConnection&, uint64_t, uint64_t id)
{
    m_nodemanager.AddNode(id, CThreadedNode(id, conn.GetNetConfig(), false));
    return true;
}

void CThreadedConnManager::OnBindFailure(const CConnectionListener&, uint64_t, bool, uint64_t)
{
}

void CThreadedConnManager::OnBind(const CConnectionListener&, uint64_t, uint64_t)
{
}

bool CThreadedConnManager::OnNeedIncomingListener(CConnectionListener&, uint64_t)
{
    return false;
}

void CThreadedConnManager::OnDisconnected(const CConnection&, uint64_t id, bool, uint64_t)
{
    m_nodemanager.Remove(id);
}

void CThreadedConnManager::OnMalformedMessage(uint64_t id)
{
    CloseConnection(id, true);
}
