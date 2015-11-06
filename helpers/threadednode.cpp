// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "threadednode.h"

CThreadedNode::CThreadedNode()
: m_id(0), m_incoming(false), nSendVersion(0), nRecvVersion(0){}

CThreadedNode::CThreadedNode(uint64_t id, const CNetworkConfig& netconfig, bool incoming)
: m_id(id), m_incoming(incoming), nSendVersion(0), nRecvVersion(0), m_netconfig(netconfig){}

CThreadedNode::~CThreadedNode()
{
}

void CThreadedNode::SetSendVersion(int version)
{
    nSendVersion = version;
}

void CThreadedNode::UpgradeRecvVersion()
{
    nRecvVersion = nSendVersion;
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

CNetworkConfig CThreadedNode::GetNetConfig() const
{
    return m_netconfig;
}
