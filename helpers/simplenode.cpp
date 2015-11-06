// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simplenode.h"

CSimpleNode::CSimpleNode()
: m_id(0), m_incoming(false), nSendVersion(0), nRecvVersion(0){}

CSimpleNode::CSimpleNode(uint64_t id, const CConnection& connection, bool incoming)
: m_id(id), m_incoming(incoming), nSendVersion(connection.GetNetConfig().protocol_handshake_version), nRecvVersion(nSendVersion), m_connection(connection){}

CSimpleNode::~CSimpleNode()
{
}

void CSimpleNode::SetOurVersion(const CMessageVersion& version)
{
    m_our_version = version;
}

void CSimpleNode::SetTheirVersion(const CMessageVersion& version)
{
    m_their_version = version;
}

void CSimpleNode::UpgradeSendVersion()
{
     nSendVersion = std::min(m_our_version.nVersion, m_their_version.nVersion);
}

void CSimpleNode::UpgradeRecvVersion()
{
    nRecvVersion = std::min(m_our_version.nVersion, m_their_version.nVersion);
}

uint64_t CSimpleNode::GetId() const
{
    return m_id;
}

int CSimpleNode::GetRecvVersion() const
{
    return nRecvVersion;
}

int CSimpleNode::GetSendVersion() const
{
    return nSendVersion;
}

bool CSimpleNode::IsIncoming() const
{
    return m_incoming;
}

const CConnection& CSimpleNode::GetConnection() const
{
    return m_connection;
}
