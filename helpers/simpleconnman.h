// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_SIMPLECONNMAN_H
#define BITCOIN_NET_SIMPLECONNMAN_H

#include "libbtcnet/handler.h"
#include "simpleaddrman.h"
#include "simplenodemanager.h"

class CSimpleNode;

class CSimpleConnManager : public CConnectionHandler
{
protected:
    virtual void ProcessFirstMessage(CSimpleNode* node, const std::vector<unsigned char>& inmsg) = 0;
    virtual void ProcessMessage(CSimpleNode* node, const std::vector<unsigned char>& inmsg) = 0;
    virtual void OnReadyForFirstSend(CSimpleNode* node) = 0;
    bool OnIncomingConnection(const CConnectionListener& listenconn, const CConnection& resolved_conn, uint64_t id, int incount, int totalincount);
    bool OnOutgoingConnection(const CConnection& conn, const CConnection& resolved_conn, uint64_t attemptId, uint64_t id);
    void OnConnectionFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid);
    void OnDisconnected(const CConnection& conn, uint64_t id, bool persistent, uint64_t retryid);
    void OnBindFailure(const CConnectionListener& listener, uint64_t id, bool retry, uint64_t retryid);
    void OnBind(const CConnectionListener& listener, uint64_t attemptId, uint64_t id);
    bool OnNeedIncomingListener(CConnectionListener&, uint64_t);
    void OnDnsFailure(const CConnection& conn);
    void OnWriteBufferFull(uint64_t id, size_t bufsize);
    void OnWriteBufferReady(uint64_t id, size_t bufsize);
    bool OnReceiveMessages(uint64_t id, CNodeMessages& msgs);
    void OnMalformedMessage(uint64_t id);
    void OnReadyForFirstSend(const CConnection& conn, uint64_t id);
    void OnProxyConnected(uint64_t id, const CConnection& conn);
    void OnProxyFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid);
    void OnBytesRead(uint64_t id, size_t bytes, size_t total_bytes);
    void OnBytesWritten(uint64_t id, size_t bytes, size_t total_bytes);
public:
    CSimpleConnManager(size_t max_queue_size);
    void AddSeed(const CConnection& conn);
    void AddAddress(const CConnection& conn);
    bool GetAddress(CConnection& conn);
    void AddListener(const CConnectionListener& listener);
    void Start(int outgoing_limit, int bind_limit);
    void Stop();
private:
    std::map<uint64_t, CConnection> connections_connecting;
    std::map<uint64_t, CNodeMessages> m_messages;
    CSimpleAddrMan m_addrmanager;
    CSimpleNodeManager m_nodemanager;
    bool m_stop;
    std::set<uint64_t> m_ready_for_first_send;
    size_t m_max_queue_size;
    std::list<CConnectionListener> m_listeners;
};

#endif // BITCOIN_NET_SIMPLECONNMAN_H
