// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_SIMPLENODE_H
#define BITCOIN_NET_SIMPLENODE_H

#include "libbtcnet/networkconfig.h"

#include <stdint.h>
class CSimpleNode
{
public:
    CSimpleNode();
    CSimpleNode(uint64_t id, const CNetworkConfig& netconfig, bool incoming);
    virtual ~CSimpleNode();
    void UpgradeRecvVersion();
    void SetSendVersion(int version);

    uint64_t GetId() const;
    int GetRecvVersion() const;
    int GetSendVersion() const;
    bool IsIncoming() const;
private:
    uint64_t m_id;
    bool m_incoming;
    int nSendVersion;
    int nRecvVersion;
    CNetworkConfig m_netconfig;
};

#endif // BITCOIN_NET_SIMPLENODE_H
