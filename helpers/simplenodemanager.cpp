// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simplenodemanager.h"

#include <stddef.h>
#include <assert.h>

void CSimpleNodeManager::AddNode(uint64_t id, const CSimpleNode& node)
{
   CNodeHolder holder;
   holder.m_node = node;
   holder.m_ref = 1;
   holder.m_disconnected = 0;
   m_nodes[id] = holder;
}

CSimpleNode* CSimpleNodeManager::GetNode(uint64_t id)
{
    std::map<uint64_t, CNodeHolder>::iterator it = m_nodes.find(id);
    if(it != m_nodes.end())
    {
        if(it->second.m_disconnected)
            return NULL;
        it->second.m_ref++;
        return &it->second.m_node;
    }
    return NULL;
}
void CSimpleNodeManager::Unref(uint64_t id)
{
    std::map<uint64_t, CNodeHolder>::iterator it = m_nodes.find(id);
    if(it != m_nodes.end())
    {
        assert(it->second.m_ref);
        if(!--it->second.m_ref)
            m_nodes.erase(it);
    }
}
void CSimpleNodeManager::Remove(uint64_t id)
{
    std::map<uint64_t, CNodeHolder>::iterator it = m_nodes.find(id);
    if(it != m_nodes.end())
    {
        assert(!it->second.m_disconnected);
        assert(it->second.m_ref);
        if(!--it->second.m_ref)
            m_nodes.erase(it);
        else
            it->second.m_disconnected = true;
    }
}
