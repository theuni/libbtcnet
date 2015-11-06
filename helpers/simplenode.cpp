// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simplenode.h"

CSimpleNode::CSimpleNode()
: m_id(0), m_incoming(false), nSendVersion(0), nRecvVersion(0){}

CSimpleNode::CSimpleNode(uint64_t id, const CNetworkConfig& config, bool incoming)
: m_id(id), m_incoming(incoming), nSendVersion(0), nRecvVersion(0), m_netconfig(config){}

CSimpleNode::~CSimpleNode()
{
}

void CSimpleNode::SetSendVersion(int version)
{
    nSendVersion = version;
}

void CSimpleNode::UpgradeRecvVersion()
{
    nRecvVersion = nSendVersion;
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
