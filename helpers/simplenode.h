// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_SIMPLENODE_H
#define BITCOIN_NET_SIMPLENODE_H

#include "libbtcnet/connection.h"
#include "bitcoin/serialize/version.h"

#include <stdint.h>

class CSimpleNode
{
public:
    CSimpleNode();
    CSimpleNode(uint64_t id, const CConnection& connection, bool incoming);
    virtual ~CSimpleNode();
    void UpgradeRecvVersion();
    void UpgradeSendVersion();

    void SetOurVersion(const CMessageVersion& version);
    void SetTheirVersion(const CMessageVersion& version);

    uint64_t GetId() const;
    int GetRecvVersion() const;
    int GetSendVersion() const;
    bool IsIncoming() const;
    const CConnection& GetConnection() const;
private:
    uint64_t m_id;
    bool m_incoming;
    int nSendVersion;
    int nRecvVersion;
    CConnection m_connection;
    CMessageVersion m_our_version;
    CMessageVersion m_their_version;
};

#endif // BITCOIN_NET_SIMPLENODE_H
