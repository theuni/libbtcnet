// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADEDNODEMAN_H
#define BITCOIN_THREADEDNODEMAN_H

#ifdef USE_CXX11
#include <mutex>
#include <condition_variable>
#include <thread>
typedef std::mutex base_mutex_type;
typedef std::lock_guard<base_mutex_type> lock_guard_type;
typedef std::unique_lock<base_mutex_type> unique_lock_type;
typedef std::condition_variable condition_variable_type;
typedef std::thread thread_type;
#else
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
typedef boost::mutex base_mutex_type;
typedef boost::lock_guard<base_mutex_type> lock_guard_type;
typedef boost::unique_lock<base_mutex_type> unique_lock_type;
typedef boost::condition_variable condition_variable_type;
typedef boost::thread thread_type;
#endif

#include <map>
#include <assert.h>

template <typename NodeType>
class CThreadedNodeManager
{
    template <typename>
    struct CNodeHolder
    {
        CNodeHolder(const NodeType& node) : m_node(node), m_refcount(0), m_remove(false), m_disconnected(false){};
        NodeType m_node;
        int m_refcount;
        bool m_remove;
        bool m_disconnected;
    };
public:
    typedef CNodeHolder<NodeType> HolderType;
    typedef std::map<uint64_t, HolderType*> NodeMapType;
    typedef typename NodeMapType::iterator NodeMapIter;

    ~CThreadedNodeManager()
    {
        for(NodeMapIter it = m_nodes.begin(); it != m_nodes.end(); ++it)
            delete it->second;
    }

    bool AddNode(uint64_t id, const NodeType& nodeIn)
    {
        const std::pair<uint64_t, HolderType*> node(id, new HolderType(nodeIn));
        lock_guard_type lock(m_nodes_mutex);
        return m_nodes.insert(node).second;
    }

    NodeType* GetNode(uint64_t id)
    {
        lock_guard_type lock(m_nodes_mutex);
        NodeMapIter it = m_nodes.find(id);
        if(it != m_nodes.end() && !it->second->m_remove)
        {
            HolderType* holder = it->second;
            holder->m_refcount++;
            return &holder->m_node;
        }
        return NULL;
    }

    void Remove(uint64_t id)
    {
        lock_guard_type lock(m_nodes_mutex);
        NodeMapIter it = m_nodes.find(id);
        if(it != m_nodes.end())
        {
            HolderType* holder = it->second;
            assert(!holder->m_remove);
            holder->m_remove = true;
            if(!holder->m_refcount)
            {
                delete holder;
                m_nodes.erase(it);
            }
        }
    }

    void SetDisconnected(uint64_t id)
    {
        lock_guard_type lock(m_nodes_mutex);
        NodeMapIter it = m_nodes.find(id);
        if(it != m_nodes.end())
        {
            HolderType* holder = it->second;
            assert(!holder->m_disconnected);
            holder->m_disconnected = true;
        }
    }

    int AddRef(uint64_t id)
    {
        lock_guard_type lock(m_nodes_mutex);
        NodeMapIter it = m_nodes.find(id);
        if(it != m_nodes.end())
        {
            HolderType* holder = it->second;
            return holder->m_refcount++;
        }
        return 0;
    }

    int Release(uint64_t id)
    {
        lock_guard_type lock(m_nodes_mutex);
        NodeMapIter it = m_nodes.find(id);
        if(it == m_nodes.end())
            return 0;
        HolderType* holder = it->second;
        int refcount = holder->m_refcount--;
        if(!refcount && (holder->m_remove == true))
        {
            delete holder;
            m_nodes.erase(it);
        }
        return refcount;
    }
    
private:

    NodeMapType m_nodes;
    base_mutex_type m_nodes_mutex;
};

#endif
