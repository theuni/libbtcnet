// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "threadedaddrman.h"

void CThreadedAddrMan::Add(std::list<CConnection>& conns)
{
    lock_guard_type lock(m_mutex);
    m_connections_available.splice(m_connections_available.end(), conns, conns.begin(), conns.end());
}

void CThreadedAddrMan::Add(const CConnection& conn)
{
    lock_guard_type lock(m_mutex);
    m_connections_available.push_back(conn);
}

bool CThreadedAddrMan::Get(CConnection& conn)
{
    lock_guard_type lock(m_mutex);
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

void CThreadedAddrMan::AddSeed(const CConnection& conn)
{
    lock_guard_type lock(m_mutex);
    m_seeds.push_back(conn);
}
