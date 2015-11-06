// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_THREADEDADDRMAN_H
#define BITCOIN_NET_THREADEDADDRMAN_H

#include "libbtcnet/connection.h"

#include <list>

class CThreadedAddrMan
{
public:
    void Add(const CConnection& conn);
    CConnection Get();
    void AddSeed(const CConnection& conn);
private:
    std::list<CConnection> m_connections_available;
    std::list<CConnection> m_seeds;
};

#endif // BITCOIN_NET_THREADEDADDRMAN_H
