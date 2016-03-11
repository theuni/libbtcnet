// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_HANDLER_H
#define LIBBTCNET_SRC_HANDLER_H

#include "libbtcnet/handler.h"
#include "threads.h"
#include "event.h"

#include <event2/bufferevent.h>
#include <event2/util.h>
#include <map>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <vector>

#ifndef NO_THREADS
#include <thread>
#endif

enum ConnectionFailureType {
    NONE = 0,
    CONNECT = 1 << 0,
    RESOLVE = 1 << 1,
    PROXY = 1 << 2
};

struct bufferevent;
struct event_base;
struct ev_token_bucket_cfg;
struct bufferevent_rate_limit_group;
struct evdns_base;
struct event;

class ConnectionBase;
class CResolveOnly;
class CConnectionBase;
class CConnListener;
class CConnectionHandlerInt
{
    friend class ConnectionBase;
    friend class CConnListener;
    friend class CResolveOnly;

public:
    CConnectionHandlerInt(CConnectionHandler& handler, bool enable_threading);
    ~CConnectionHandlerInt();

    void SetIncomingRateLimit(const CRateLimit& limit);
    void SetOutgoingRateLimit(const CRateLimit& limit);
    void CloseConnection(ConnID id, bool immediately);
    bool Send(ConnID id, const unsigned char* data, size_t size);
    void SetRateLimit(ConnID id, const CRateLimit& limit);
    void PauseRecv(ConnID id);
    void UnpauseRecv(ConnID id);
    bool Bind(CConnection conn);
    void Disconnect(ConnID id, bool force);
    bool PumpEvents(bool block);
    void Shutdown();
    void Start(int outgoing_limit);

    bufferevent_options GetBevOpts() const;
    const event_type<evdns_base>& GetDNSBase() const;
    const event_type<event_base>& GetEventBase() const;

private:
    bool OnReceiveMessages(ConnID id, std::list<std::vector<unsigned char> >&& msgs, size_t totalsize);
    void OnIncomingConnected(ConnID id, const CConnection& conn, const CConnection& resolved_conn);
    void OnOutgoingConnected(ConnID id, const CConnection& conn, const CConnection& resolved_conn);
    void OnConnectionFailure(ConnID id, ConnectionFailureType type, int error, CConnection failed, bool retry);
    void OnWriteBufferFull(ConnID id, size_t bufsize);
    void OnWriteBufferReady(ConnID id, size_t bufsize);
    void OnResolveComplete(ConnID id, const CConnection& conn, std::list<CConnection> resolved);
    void OnResolveFailure(ConnID id, const CConnection& conn, int error, bool retry);
    void OnIncomingConnection(const CConnection& bind, evutil_socket_t sock, sockaddr* address, int socklen);
    void OnListenFailure(ConnID id, const CConnection& bind);
    void OnDisconnected(ConnID id, bool reconnect);

    void RequestOutgoingInt();
    void ShutdownInt();
    void BindInt();

    void StartConnection(CConnection&& conn);
    void SetSocketOpts(sockaddr* addr, int socksize, evutil_socket_t sock);
    bool IsEventThread() const;

    std::map<ConnID, std::unique_ptr<ConnectionBase> > m_connected;
    std::map<ConnID, std::unique_ptr<ConnectionBase> > m_connecting;
    std::map<ConnID, std::unique_ptr<CConnListener> > m_binds;
    std::map<ConnID, std::unique_ptr<CResolveOnly> > m_dns_resolves;

    CConnectionHandler& m_interface;
    ConnID m_connection_index;

    size_t m_bytes_read;
    size_t m_bytes_written;

    int m_outgoing_conn_count;
    int m_incoming_conn_count;

    int m_outgoing_conn_limit;

    bool m_enable_threading;
    bool m_shutdown;

#ifndef NO_THREADS
    std::mutex m_conn_mutex;
    std::mutex m_bind_mutex;
    std::mutex m_group_rate_mutex;
    std::thread::id m_main_thread;
#endif

    event_type<ev_token_bucket_cfg> m_incoming_rate_cfg;
    event_type<ev_token_bucket_cfg> m_outgoing_rate_cfg;

    event_type<bufferevent_rate_limit_group> m_incoming_rate_limit;
    event_type<bufferevent_rate_limit_group> m_outgoing_rate_limit;

    event_type<event_base> m_event_base;
    event_type<evdns_base> m_dns_base;

    CEvent m_request_event;
    CEvent m_shutdown_event;
};

#endif // LIBBTCNET_SRC_HANDLER_H
