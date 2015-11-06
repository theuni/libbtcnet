// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_HANDLER_H
#define BITCOIN_NET_HANDLER_H

#include <map>
#include <vector>
#include <deque>
#include <list>
#include <stdint.h>
#include <stddef.h>

struct sockaddr;
struct fd_type;
struct mutex_holder;
struct thread_holder;

class CConnection;
class CConnectionListener;
class CRateLimit;

struct connection_data;
struct listener_data;

struct event_base;
struct ev_token_bucket_cfg;
struct bufferevent_rate_limit_group;
struct evdns_base;
struct event;

//typedef std::list<std::vector<unsigned char> > CNodeMessages;

struct CNodeMessages
{
    std::list<std::vector<unsigned char> > messages;
    size_t size;
    bool first_receive;
};

struct CReceivedMessage
{
    std::vector<unsigned char> vRecvMsg;
    uint64_t recvTime;
};

typedef unsigned int ConnID;

class CConnectionHandler
{
    friend struct connection_callbacks;
    friend struct proxy_callbacks;
    friend struct timer_callbacks;
    friend struct dns_callbacks;
    friend struct bev_callbacks;
    friend struct buf_callbacks;
public:
    void SetIncomingRateLimit(const CRateLimit& limit);
    void SetOutgoingRateLimit(const CRateLimit& limit);
    void CloseConnection(uint64_t id, bool immediately);
    bool SendMsg(uint64_t id, const unsigned char* data, size_t size);
    bool SetRateLimit(uint64_t id, const CRateLimit& limit);
    int PauseRecv(uint64_t id);
    int UnpauseRecv(uint64_t id);

protected:
    CConnectionHandler(bool enable_threading);
    virtual ~CConnectionHandler();

    int PumpEvents(bool block);
    void Shutdown();
    void Start(int outgoing_limit, int bind_limit);

    virtual bool OnNeedOutgoingConnections(std::vector<CConnection>& connection, int needcount) = 0;
    virtual bool OnNeedIncomingListener(CConnectionListener& connection, uint64_t id) = 0;

    virtual void OnBind(const CConnectionListener& listener, uint64_t attemptId, uint64_t id) = 0;
    virtual void OnBindFailure(const CConnectionListener& listener, uint64_t id, bool retry, uint64_t retryid) = 0;

    virtual void OnDnsResponse(const CConnection& conn, uint64_t id, std::list<CConnection>& results) = 0;
    virtual void OnDnsFailure(const CConnection& conn) = 0;

    virtual bool OnOutgoingConnection(const CConnection& conn, const CConnection& resolved_conn, uint64_t attemptId, uint64_t id) = 0;
    virtual void OnConnectionFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid) = 0;
    virtual bool OnIncomingConnection(const CConnectionListener& listenconn,  const CConnection& resolved_conn, uint64_t id, int incount, int totalincount) = 0;
    virtual void OnDisconnected(const CConnection& conn, uint64_t id, bool persistent, uint64_t retryid) = 0;
    virtual void OnReadyForFirstSend(const CConnection& conn, uint64_t id) = 0;
    virtual bool OnReceiveMessages(uint64_t id, CNodeMessages& msgs) = 0;
    virtual void OnMalformedMessage(uint64_t id) = 0;
    virtual void OnWriteBufferFull(uint64_t id, size_t bufsize) = 0;
    virtual void OnWriteBufferReady(uint64_t id, size_t bufsize) = 0;
    virtual void OnProxyConnected(uint64_t id, const CConnection& conn) = 0;
    virtual void OnProxyFailure(const CConnection& conn, const CConnection& resolved_conn, uint64_t id, bool retry, uint64_t retryid) = 0;
    virtual void OnBytesRead(uint64_t id, size_t bytes, size_t total_bytes) = 0;
    virtual void OnBytesWritten(uint64_t id, size_t bytes, size_t total_bytes) = 0;
private:

    typedef std::map<uint64_t, connection_data> connections_list_type;
    typedef connections_list_type::iterator conn_iter;
    typedef std::map<uint64_t, listener_data> bind_list_type;
    typedef bind_list_type::iterator bind_iter;

    void ActivateConnectionsTimer();
    void SetSocketOpts(sockaddr* addr, int socksize, const fd_type& sock);
    void AddConnection(const connection_data& data);
    void DisconnectNow(uint64_t id);
    void DisconnectAfterWrite(uint64_t id);
    void HandleDisconnected(const connection_data& old_data_copy);
    void IncomingConnected(listener_data* listen, const CConnection& conn, const fd_type& sock);
    void OutgoingConnectionFailure(uint64_t id);
    void LookupResults(uint64_t id, std::list<CConnection>& results);
    void ReceiveMessages(connection_data* data);
    void ProxyConnected(connection_data* data);
    void OutgoingConnected(uint64_t id);
    void BindFailure(listener_data* data);
    void Disconnected(uint64_t id);
    void BytesRead(connection_data* data, size_t prev_size, size_t bytes);
    void BytesWritten(connection_data* data, size_t prev_size, size_t bytes);
    void WriteBufferReady(connection_data* data);

    connection_data* CreateRetry(const connection_data& olddata);
    connection_data* CreateOutgoing(const CConnection& conn);
    bool IsEventThread() const;
    bool ConnectOutgoing(connection_data*);
    bool ConnectListener(uint64_t id);
    void DisconnectInt(uint64_t id, bool immediately);
    void RequestOutgoing();
    void RequestBind();
    void CreateListener(const CConnectionListener& listener);
    void AddTimedConnect(connection_data* data, int seconds);
    void DeleteListener(listener_data*);

    static void SetGroupRateLimit(const CRateLimit& limit, ev_token_bucket_cfg*& cfg, bufferevent_rate_limit_group* group);

    connections_list_type m_connected;
    uint64_t m_connected_index;

    bind_list_type m_binds;
    uint64_t m_bind_index;

    bind_list_type m_bind_attempts;
    uint64_t m_bind_attempt_index;

    connections_list_type m_outgoing_attempts;
    uint64_t m_outgoing_attempt_index;

    size_t m_bytes_read;
    size_t m_bytes_written;

    int m_outgoing_conn_count;
    int m_incoming_conn_count;

    int m_outgoing_conn_limit;
    int m_bind_limit;

    bool m_enable_threading;
    bool m_shutdown;

    mutex_holder* m_conn_mutex;
    mutex_holder* m_rate_mutex;

    thread_holder* m_main_thread;

    ev_token_bucket_cfg* m_incoming_rate_cfg;
    ev_token_bucket_cfg* m_outgoing_rate_cfg;

    bufferevent_rate_limit_group* m_incoming_rate_limit;
    bufferevent_rate_limit_group* m_outgoing_rate_limit;

    event_base* m_event_base;
    evdns_base* m_dns_base;

    event* m_request_event;
};

#endif // BITCOIN_NET_HANDLER_H
