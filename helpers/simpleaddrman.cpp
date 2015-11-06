// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simpleaddrman.h"

void CSimpleAddrMan::Add(const CConnection& conn)
{
    m_connections_available.push_back(conn);
}

CConnection CSimpleAddrMan::Get()
{
    CConnection ret;
    if(!m_connections_available.empty())
    {
        ret = m_connections_available.front();
        m_connections_available.pop_front();
    }
    else if(!m_seeds.empty())
    {
        ret = m_seeds.front();
        m_seeds.pop_front();
    }
    return ret;
}

void CSimpleAddrMan::AddSeed(const CConnection& conn)
{
    m_seeds.push_back(conn);
}
