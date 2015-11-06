// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_SIMPLENODEMANAGER_H
#define BITCOIN_NET_SIMPLENODEMANAGER_H

#include "simplenode.h"
#include <map>

class CSimpleNodeManager
{
public:
    void AddNode(uint64_t id, const CSimpleNode& node);
    CSimpleNode* GetNode(uint64_t id);
    void  Unref(uint64_t id);
    void Remove(uint64_t id);
private:
    struct CNodeHolder
    {
        CSimpleNode m_node;
        int m_ref;
        bool m_disconnected;
    };
    std::map<uint64_t, CNodeHolder> m_nodes;
};

#endif // BITCOIN_NET_SIMPLENODEMANAGER_H
