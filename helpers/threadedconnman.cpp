// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "threadedconnman.h"
#include "threadednode.h"

#include <set>
#include <list>

CThreadedConnManager::CThreadedConnManager(size_t max_queue_size)
: CConnectionHandler(true), m_stop(false), m_max_queue_size(max_queue_size)
{}

void CThreadedConnManager::Stop()
{
    {
        lock_guard_type lock(m_mutex);
        m_stop = true;
    }
    m_condvar.notify_one();
}

void CThreadedConnManager::Start(int outgoing_limit, int bind_limit)
{
    {
        lock_guard_type lock(m_mutex);
        m_stop = false;
    }
    CConnectionHandler::Start(outgoing_limit, bind_limit);
    while(1)
    {
        {
            lock_guard_type lock(m_mutex);
            if(m_stop)
                break;
        }
        PumpEvents(true);
    }
    CConnectionHandler::Shutdown();

    {
        lock_guard_type lock(m_mutex);
        m_messages.clear();
        m_message_queue.clear();
        m_ready_for_first_send.clear();
    }
}

void CThreadedConnManager::Run()
{
    std::map<uint64_t, std::vector<unsigned char> > messages;
    std::set<uint64_t> first_send;
    std::set<uint64_t> first_receive;
    while(true)
    {
        {
            unique_lock_type lock(m_mutex);
            while(!m_stop && m_message_queue.empty() && m_ready_for_first_send.empty())
                m_condvar.wait(lock);
            if(m_stop)
                break;

            for(std::map<uint64_t, CNodeMessages>::iterator i = m_message_queue.begin(); i != m_message_queue.end();)
            {
                uint64_t id = i->first;
                if(i->second.size)
                {
                    messages[id] = i->second.messages.front();
                    size_t msgsize = i->second.messages.front().size();

                    if(i->second.size >= m_max_queue_size && i->second.size - msgsize < m_max_queue_size)
                        UnpauseRecv(id);

                    i->second.size -= msgsize;

                    if(i->second.first_receive)
                    {
                        first_receive.insert(i->first);
                        i->second.first_receive = false;
                    }

                    if(!i->second.size)
                    {
                        m_message_queue.erase(i++);
                    }
                    else
                    {
                        i->second.messages.pop_front();
                        ++i;
                    }
                }
            }

            if(!m_ready_for_first_send.empty())
            {
                m_ready_for_first_send.swap(first_send);
            }
        }

        if(!first_send.empty())
        {
            for(std::set<uint64_t>::const_iterator i = first_send.begin(); i != first_send.end(); ++i)
            {
                CThreadedNode* node = m_nodemanager.GetNode(*i);
                if(node)
                {
                    OnReadyForFirstSend(node);
                    m_nodemanager.Release(*i);
                }
            }
            first_send.clear();
        }

        if(!messages.empty())
        {
            for(std::map<uint64_t, std::vector<unsigned char> >::iterator i = messages.begin(); i != messages.end();++i)
            {
                uint64_t id = i->first;
                CThreadedNode* node = m_nodemanager.GetNode(id);
                if(node)
                {
                    if(first_receive.count(i->first))
                    {
                        first_receive.erase(i->first);
                        ProcessFirstMessage(node, i->second);
                    }
                    else
                        ProcessMessage(node, i->second);

                    m_nodemanager.Release(id);
                }
            }
            messages.clear();
        }
    }
}

bool CThreadedConnManager::OnReceiveMessages(uint64_t id, CNodeMessages& msgs)
{
    bool notify = false;
    {
        lock_guard_type lock(m_mutex);
        notify = m_message_queue.empty();
        std::map<uint64_t, CNodeMessages>::iterator it = m_message_queue.find(id);
        if(it == m_message_queue.end())
        {
            it = m_message_queue.insert(std::make_pair(id, msgs)).first;
        }
        else
        {
            it->second.messages.splice(it->second.messages.end(), msgs.messages, msgs.messages.begin(), msgs.messages.end());
            it->second.size += msgs.size;
        }

        if(it->second.size < m_max_queue_size && it->second.size + msgs.size >= m_max_queue_size)
            PauseRecv(id);
    }
    if(notify)
        m_condvar.notify_one();
    return true;
}

void CThreadedConnManager::OnReadyForFirstSend(const CConnection& conn, uint64_t id)
{
    bool notify = false;
    {
        lock_guard_type lock(m_mutex);
        notify = m_ready_for_first_send.empty();
        m_ready_for_first_send.insert(id);
    }
    if(notify)
        m_condvar.notify_one();
}

void CThreadedConnManager::AddSeed(const CConnection& conn)
{
    m_addrmanager.AddSeed(conn);
}

void CThreadedConnManager::AddAddress(std::list<CConnection>& conns)
{
    m_addrmanager.Add(conns);
}

void CThreadedConnManager::AddAddress(const CConnection& conn)
{
    m_addrmanager.Add(conn);
}

void CThreadedConnManager::AddListener(const CConnectionListener& conn)
{
    m_listeners.push_back(conn);
}

bool CThreadedConnManager::GetAddress(CConnection& conn)
{
    return m_addrmanager.Get(conn);
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

bool CThreadedConnManager::OnIncomingConnection(const CConnectionListener&, const CConnection& resolved_conn, uint64_t id, int incount, int totalincount)
{
    m_nodemanager.AddNode(id, CThreadedNode(id, resolved_conn, true));
    return true;
}

bool CThreadedConnManager::OnOutgoingConnection(const CConnection& conn, const CConnection& resolved_conn, uint64_t, uint64_t id)
{
    m_nodemanager.AddNode(id, CThreadedNode(id, resolved_conn, false));
    return true;
}

void CThreadedConnManager::OnBindFailure(const CConnectionListener&, uint64_t, bool, uint64_t)
{
}

void CThreadedConnManager::OnBind(const CConnectionListener&, uint64_t, uint64_t)
{
}

bool CThreadedConnManager::OnNeedIncomingListener(CConnectionListener& listener, uint64_t)
{
    if(!m_listeners.empty())
    {
        listener = m_listeners.front();
        m_listeners.pop_front();
        return true;
    }
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

void CThreadedConnManager::OnProxyConnected(uint64_t id, const CConnection& conn)
{
}

void CThreadedConnManager::OnProxyFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid)
{
}

void CThreadedConnManager::OnBytesRead(uint64_t id, size_t bytes, size_t total_bytes)
{
}

void CThreadedConnManager::OnBytesWritten(uint64_t id, size_t bytes, size_t total_bytes)
{
}
