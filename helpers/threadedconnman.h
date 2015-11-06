// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_THREADEDCONNMAN_H
#define BITCOIN_NET_THREADEDCONNMAN_H

#include "libbtcnet/handler.h"
#include "threadednodeman.h"
#include "threadedaddrman.h"
#include <set>
class CThreadedNode;

class CThreadedConnManager : public CConnectionHandler
{
protected:
    virtual ~CThreadedConnManager(){}
    virtual void ProcessFirstMessage(CThreadedNode* node, const std::vector<unsigned char>& inmsg) = 0;
    virtual void ProcessMessage(CThreadedNode* node, const std::vector<unsigned char>& inmsg) = 0;
    virtual void OnReadyForFirstSend(CThreadedNode* node) = 0;
    bool OnIncomingConnection(const CConnectionListener& listenconn, const CConnection& resolved_conn, uint64_t id, int incount, int totalincount);
    bool OnOutgoingConnection(const CConnection& conn, const CConnection& resolved_conn, uint64_t attemptId, uint64_t id);
    void OnConnectionFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid);
    void OnReadyForFirstSend(const CConnection& conn, uint64_t id);
    void OnDisconnected(const CConnection& conn, uint64_t id, bool persistent, uint64_t retryid);
    void OnBindFailure(const CConnectionListener& listener, uint64_t id, bool retry, uint64_t retryid);
    void OnBind(const CConnectionListener& listener, uint64_t attemptId, uint64_t id);
    bool OnNeedIncomingListener(CConnectionListener&, uint64_t);
    void OnDnsFailure(const CConnection& conn);
    void OnWriteBufferFull(uint64_t id, size_t bufsize);
    void OnWriteBufferReady(uint64_t id, size_t bufsize);
    bool OnReceiveMessages(uint64_t id, CNodeMessages& msgs);
    void OnMalformedMessage(uint64_t id);
    void OnProxyConnected(uint64_t id, const CConnection& conn);
    void OnProxyFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid);
    void OnBytesRead(uint64_t id, size_t bytes, size_t total_bytes);
    void OnBytesWritten(uint64_t id, size_t bytes, size_t total_bytes);
public:
    CThreadedConnManager(size_t max_queue_size);
    void AddSeed(const CConnection& conn);
    void AddAddress(std::list<CConnection>& conns);
    void AddAddress(const CConnection& conn);
    void AddListener(const CConnectionListener& listener);
    bool GetAddress(CConnection& conn);
    void Start(int outgoing_limit, int bind_limit);
    void Run();
    void Stop();
private:
    std::map<uint64_t, CConnection> connections_connecting;
    std::map<uint64_t, CNodeMessages> m_messages;
    bool m_stop;
    CThreadedAddrMan m_addrmanager;
    CThreadedNodeManager<CThreadedNode> m_nodemanager;
    base_mutex_type m_mutex;
    condition_variable_type m_condvar;
    std::map<uint64_t, CNodeMessages> m_message_queue;
    std::set<uint64_t> m_ready_for_first_send;
    size_t m_max_queue_size;
    std::list<CConnectionListener> m_listeners;
};

#endif // BITCOIN_NET_THREADEDCONNMAN_H
