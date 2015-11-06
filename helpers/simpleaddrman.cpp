// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simpleaddrman.h"

void CSimpleAddrMan::Add(const CConnection& conn)
{
    m_connections_available.push_back(conn);
}

bool CSimpleAddrMan::Get(CConnection& conn)
{
    if(!m_connections_available.empty())
    {
        conn = m_connections_available.front();
        m_connections_available.pop_front();
        return true;
    }
    else if(!m_seeds.empty())
    {
        conn = m_seeds.front();
        m_seeds.pop_front();
        return true;
    }
    return false;
}

void CSimpleAddrMan::AddSeed(const CConnection& conn)
{
    m_seeds.push_back(conn);
}
