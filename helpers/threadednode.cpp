// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "threadednode.h"

CThreadedNode::CThreadedNode()
: m_id(0), m_incoming(false), nSendVersion(0), nRecvVersion(0){}

CThreadedNode::CThreadedNode(uint64_t id, const CConnection& connection, bool incoming)
: m_id(id), m_incoming(incoming), nSendVersion(connection.GetNetConfig().protocol_handshake_version), nRecvVersion(nSendVersion), m_connection(connection){}

CThreadedNode::~CThreadedNode()
{
}

void CThreadedNode::SetOurVersion(const CMessageVersion& version)
{
    m_our_version = version;
}

void CThreadedNode::SetTheirVersion(const CMessageVersion& version)
{
    m_their_version = version;
}

void CThreadedNode::UpgradeSendVersion()
{
     nSendVersion = std::min(m_our_version.nVersion, m_their_version.nVersion);
}

void CThreadedNode::UpgradeRecvVersion()
{
    nRecvVersion = std::min(m_our_version.nVersion, m_their_version.nVersion);
}

uint64_t CThreadedNode::GetId() const
{
    return m_id;
}

int CThreadedNode::GetRecvVersion() const
{
    return nRecvVersion;
}

int CThreadedNode::GetSendVersion() const
{
    return nSendVersion;
}

bool CThreadedNode::IsIncoming() const
{
    return m_incoming;
}

const CConnection& CThreadedNode::GetConnection() const
{
    return m_connection;
}
