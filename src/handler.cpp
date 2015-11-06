#include "libbtcnet/handler.h"
#include "libbtcnet/connection.h"

#include "connection_data.h"
#include "callbacks.h"
#include "proxy.h"
#include "message.h"
#include "threading.h"

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>

#ifndef NO_THREADS
#include <event2/thread.h>
#endif

#include <assert.h>
#include <string.h>
#include <limits>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#else
#include <netinet/tcp.h>
#include <sys/un.h>
#endif

#ifndef NO_THREADS
#define optional_lock(mutex, enabled) (optional_lock_type(mutex, enabled)) 
#else
#define optional_lock(mutex, enabled)
#endif

namespace {

static void setup_threads()
{
#if !defined(NO_THREADS)
#if defined(EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED)
    evthread_use_windows_threads();
    return;
#elif defined(EVTHREAD_USE_PTHREADS_IMPLEMENTED)
    evthread_use_pthreads();
    return;
#endif
#endif
    throw std::runtime_error("Thread support requested but not compiled in.");
}

}

struct fd_type
{
    evutil_socket_t fd;
};

struct event_payload
{
    CConnectionHandler* handler;
    uint64_t id;
    event* ev;
    bool immediately;
};

struct connection_callbacks
{

    static void listen_error_cb(struct evconnlistener*, void *ctx)
    {
        if(!ctx)
            return;
        listener_data* data = static_cast<listener_data*>(ctx);
        if(data->handler)
            data->handler->DeleteListener(data);
    }

    static void accept_conn(evconnlistener* , evutil_socket_t fd, sockaddr *address, int socklen,void *ctx)
    {
        assert(ctx);
        listener_data* data = static_cast<listener_data*>(ctx);
        assert(data->handler);
        fd_type sock = {fd};
        CConnection conn(address, socklen, data->conn.GetOptions(), data->conn.GetNetConfig());
        data->handler->SetSocketOpts(address, socklen, sock);
        data->handler->IncomingConnected(data, conn, sock);
    }

    static void outgoing_conn(bufferevent *bev, short type, void *ctx)
    {
        assert(ctx);
        connection_data* data = static_cast<connection_data*>(ctx);
        if(type & BEV_EVENT_CONNECTED)
        {
            if(data->conn.GetProxy().IsSet())
            {
                data->proxy_connected = true;
                data->handler->ProxyConnected(data);
                if(data->conn.GetProxy().GetType() == CProxy::SOCKS5)
                    proxy_init(bev, data);
            }
            else
                data->handler->OutgoingConnected(data->id);
        }
        else
        {
            data->handler->OutgoingConnectionFailure(data->id);
        }
    }

    static void force_disconnect(evutil_socket_t, short, void *ctx)
    {
        event_payload* payload = static_cast<event_payload*>(ctx);
        event* ev = payload->ev;
        uint64_t id = payload->id;
        bool immediately = payload->immediately;

        CConnectionHandler* handler = payload->handler;
        if(ev)
            event_free(ev);
        delete payload;

        if(immediately)
            handler->DisconnectNow(id);
        else
            handler->DisconnectAfterWrite(id);
    }

};

CConnectionHandler::CConnectionHandler(bool enable_threading)
: m_connected_index(0)
, m_bind_index(0)
, m_bind_attempt_index(0)
, m_outgoing_attempt_index(0)
, m_bytes_read(0)
, m_bytes_written(0)
, m_outgoing_conn_count(0)
, m_incoming_conn_count(0)
, m_outgoing_conn_limit(0)
, m_bind_limit(0)
, m_enable_threading(enable_threading)
, m_shutdown(false)
, m_conn_mutex(NULL)
, m_rate_mutex(NULL)
, m_main_thread(NULL)
, m_incoming_rate_cfg(NULL)
, m_outgoing_rate_cfg(NULL)
, m_incoming_rate_limit(NULL)
, m_outgoing_rate_limit(NULL)
, m_event_base(NULL)
, m_dns_base(NULL)
, m_request_event(NULL)
{
    if(m_enable_threading)
        setup_threads();

#ifndef NO_THREADS
    m_main_thread = new thread_holder;
    m_conn_mutex = new mutex_holder;
    m_rate_mutex = new mutex_holder;
    if(m_enable_threading)
        m_main_thread->m_thread = thread_get_id();
#endif
}

CConnectionHandler::~CConnectionHandler()
{
    Shutdown();

#ifndef NO_THREADS
    if(m_conn_mutex)
    {
        delete m_conn_mutex;
        m_conn_mutex = NULL;
    }
    if(m_rate_mutex)
    {
        delete m_rate_mutex;
        m_rate_mutex = NULL;
    }
    if(m_main_thread)
    {
        delete m_main_thread;
        m_main_thread = NULL;
    }
#endif
}

void CConnectionHandler::Start(int outgoing_limit, int bind_limit)
{
    m_shutdown = false;

    if(m_enable_threading)
        m_main_thread->m_thread = thread_get_id();

    assert(m_outgoing_conn_count == 0);
    assert(m_incoming_conn_count == 0);

    m_outgoing_conn_limit = outgoing_limit;
    m_bind_limit = bind_limit;

    event_config* cfg = event_config_new();
    event_config_set_flag(cfg, EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST);
    m_event_base = event_base_new_with_config(cfg);
    event_config_free(cfg);
#if !defined(_EVENT_DISABLE_THREAD_SUPPORT) && !defined(NO_THREADS)
    if(m_enable_threading)
        evthread_make_base_notifiable(m_event_base);
#endif
    m_dns_base = evdns_base_new(m_event_base, 1);
    evdns_base_set_option(m_dns_base, "randomize-case", "0");

    {
        optional_lock(m_rate_mutex, m_enable_threading);

        if(!m_incoming_rate_cfg)
            m_incoming_rate_cfg = ev_token_bucket_cfg_new(EV_RATE_LIMIT_MAX, EV_RATE_LIMIT_MAX, EV_RATE_LIMIT_MAX, EV_RATE_LIMIT_MAX, NULL);
        m_incoming_rate_limit = bufferevent_rate_limit_group_new(m_event_base, m_incoming_rate_cfg);

        if(!m_outgoing_rate_cfg)
            m_outgoing_rate_cfg = ev_token_bucket_cfg_new(EV_RATE_LIMIT_MAX, EV_RATE_LIMIT_MAX, EV_RATE_LIMIT_MAX, EV_RATE_LIMIT_MAX, NULL);
        m_outgoing_rate_limit = bufferevent_rate_limit_group_new(m_event_base, m_outgoing_rate_cfg);
    }

    m_request_event = event_new(m_event_base, -1, EV_PERSIST, timer_callbacks::request_connections, this);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;
    event_add(m_request_event, &timeout);

    ActivateConnectionsTimer();
}

void CConnectionHandler::ActivateConnectionsTimer()
{
    if(!m_shutdown && m_request_event)
        event_active(m_request_event, EV_TIMEOUT, 0);
}

void CConnectionHandler::Shutdown()
{
    assert(IsEventThread());
    m_shutdown = true;

    if(m_request_event)
    {
        event_free(m_request_event);
        m_request_event = NULL;
    }

    if(m_dns_base)
    {
        evdns_base_free(m_dns_base, 1);
        m_dns_base = NULL;
    }

    PumpEvents(false);

    for(std::map<uint64_t, connection_data>::iterator it(m_outgoing_attempts.begin()); it != m_outgoing_attempts.end(); ++it)
    {
        if(it->second.ev)
            event_free(it->second.ev);
        if(it->second.bev)
        {
            bufferevent_disable(it->second.bev, EV_READ | EV_WRITE);
            bufferevent_setcb(it->second.bev, NULL, NULL, NULL, NULL);
            OnConnectionFailure(it->second.conn, it->second.conn, it->second.id, false, 0);
            bufferevent_free(it->second.bev);
        }
    }

    for(CConnectionHandler::connections_list_type::iterator it(m_connected.begin()); it != m_connected.end(); ++it)
    {
        bool incoming = it->second.incoming;
        bufferevent* bev = it->second.bev;
        if(bev)
        {
            bufferevent_disable(bev, EV_READ | EV_WRITE);
            bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
            bufferevent_set_rate_limit(bev, NULL);
            bufferevent_remove_from_rate_limit_group(bev);
            OnDisconnected(it->second.conn, it->second.id, false, 0);
            bufferevent_free(bev);
        }
        if(it->second.rate_cfg)
            ev_token_bucket_cfg_free(it->second.rate_cfg);
        if(it->second.ev)
            event_free(it->second.ev);
        if(incoming)
            m_incoming_conn_count--;
        else
            m_outgoing_conn_count--;
    }

    for(std::map<uint64_t, listener_data>::iterator it(m_binds.begin()); it != m_binds.end(); ++it)
        evconnlistener_free(it->second.listener);

    PumpEvents(false);

    if(m_outgoing_rate_limit)
    {
        bufferevent_rate_limit_group_free(m_outgoing_rate_limit);
        m_outgoing_rate_limit = NULL;
    }
    if(m_incoming_rate_limit)
    {
        bufferevent_rate_limit_group_free(m_incoming_rate_limit);
        m_incoming_rate_limit = NULL;
    }
    if(m_incoming_rate_cfg)
    {
        ev_token_bucket_cfg_free(m_incoming_rate_cfg);
        m_incoming_rate_cfg = NULL;
    }
    if(m_outgoing_rate_cfg)
    {
        ev_token_bucket_cfg_free(m_outgoing_rate_cfg);
        m_outgoing_rate_cfg = NULL;
    }

    if(m_event_base)
    {
        event_base_free(m_event_base);
        m_event_base = NULL;
    }

    m_bind_attempts.clear();
    m_binds.clear();
    m_outgoing_attempts.clear();
    m_connected.clear();

}

bool CConnectionHandler::IsEventThread() const
{
#ifndef NO_THREADS
    if(!m_enable_threading)
        return true;
    return thread_equal(m_main_thread);
#endif
    return true;
}

void CConnectionHandler::SetSocketOpts(sockaddr* addr, int, const fd_type& sock)
{
    evutil_make_socket_nonblocking(sock.fd);
    int set = 1;
#ifdef _WIN32
typedef char* sockoptptr;
#else
typedef void* sockoptptr;
#endif

    if(addr->sa_family == AF_INET || addr->sa_family == AF_INET6)
        setsockopt(sock.fd, IPPROTO_TCP, TCP_NODELAY, (sockoptptr)&set, sizeof(int));
}

void CConnectionHandler::IncomingConnected(listener_data* listen, const CConnection& conn, const fd_type& sock)
{
    assert(IsEventThread());

    bufferevent_options bev_opts = bufferevent_options(BEV_OPT_CLOSE_ON_FREE);
    if(m_enable_threading)
        bev_opts = bufferevent_options(bev_opts | BEV_OPT_THREADSAFE);
    bufferevent *bev = bufferevent_socket_new(m_event_base, sock.fd, bev_opts);

    if(!bev)
        return;

    bufferevent_disable(bev, EV_READ | EV_WRITE);

    uint64_t id = m_connected_index++;
    if(!OnIncomingConnection(listen->conn, conn, id, listen->incoming_connections+1, m_incoming_conn_count + 1))
    {
        bufferevent_free(bev);
        return;
    }
    listen->incoming_connections++;
    m_incoming_conn_count++;

    connection_data data;
    data.conn = conn;
    data.listen_conn = listen->id;
    data.retry = 0;
    data.bev = bev;
    data.id = id;
    data.pause_recv = 0;
    data.disconnecting = false;
    data.incoming = true;
    data.handler = this;

    AddConnection(data);
}

void CConnectionHandler::AddConnection(const connection_data& data)
{
    bufferevent* bev = data.bev;
    const CConnectionOptions& opts = data.conn.GetOptions();

    timeval initialTimeout;
    initialTimeout.tv_sec = opts.nInitialTimeout;
    initialTimeout.tv_usec = 0;

    bufferevent_set_timeouts(bev, &initialTimeout, &initialTimeout);

    {
        optional_lock(m_rate_mutex, m_enable_threading);
        if(data.incoming && m_incoming_rate_limit && m_incoming_rate_cfg)
            bufferevent_add_to_rate_limit_group(bev, m_incoming_rate_limit);
        else if (m_outgoing_rate_limit && m_outgoing_rate_cfg)
        {
            bufferevent_add_to_rate_limit_group(bev, m_outgoing_rate_limit);
        }
    }

    evbuffer* input = bufferevent_get_input(bev);
    evbuffer* output = bufferevent_get_output(bev);

    bufferevent_setwatermark(bev, EV_READ, 20, 0);
    bufferevent_setwatermark(bev, EV_WRITE, opts.nMaxSendBuffer, 0);

    connection_data* dataptr = NULL;
    const std::pair<uint64_t, connection_data>& datapair = std::make_pair(data.id, data);
    {
        optional_lock(m_conn_mutex, m_enable_threading);
        bufferevent_lock(bev);
        dataptr = &m_connected.insert(datapair).first->second;
    }
    bufferevent_setcb(bev, bev_callbacks::first_recv_cb, bev_callbacks::first_send_cb, bev_callbacks::event_cb, dataptr);
    evbuffer_add_cb(input, buf_callbacks::read_data, dataptr);
    evbuffer_add_cb(output, buf_callbacks::wrote_data, dataptr);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    bufferevent_unlock(bev);
}

void CConnectionHandler::BytesRead(connection_data* data, size_t prev_size, size_t bytes)
{
    assert(IsEventThread());
    data->bytes_read += bytes;
    m_bytes_read += bytes;
    OnBytesRead(data->id, bytes, m_bytes_read);
}

void CConnectionHandler::BytesWritten(connection_data* data, size_t prev_size, size_t bytes)
{
    assert(IsEventThread());
    data->bytes_written += bytes;
    m_bytes_written += bytes;
    OnBytesWritten(data->id, bytes, m_bytes_read);
}

void CConnectionHandler::DeleteListener(listener_data* data)
{
    if(data->listener)
    {
        evconnlistener_free(data->listener);
        data->listener = NULL;
    }
    if(data->ev)
    {
        event_free(data->ev);
        data->ev = NULL;
    }
    m_binds.erase(data->id);
}

void CConnectionHandler::ProxyConnected(connection_data* data)
{
    OnProxyConnected(data->id, data->conn);
}

void CConnectionHandler::OutgoingConnected(uint64_t attemptId)
{
    assert(IsEventThread());
    connections_list_type::iterator attempt_it = m_outgoing_attempts.find(attemptId);
    assert(attempt_it != m_outgoing_attempts.end());
 
    connection_data data = attempt_it->second;
    CConnection resolved;
    CConnectionBase baseConn = data.conn.GetProxy();
    if(!baseConn.IsSet())
        baseConn = data.conn;
    if(baseConn.IsDNS())
    {
        assert(!data.resolved_conns.empty());
        resolved = data.resolved_conns.front();
        data.resolved_conns.pop_front();
    }
    else
        resolved = data.conn;
    data.id = m_connected_index++;
    if(!OnOutgoingConnection(data.conn, resolved, attemptId, data.id))
    {
        OutgoingConnectionFailure(attemptId);
        return;
    }

    m_outgoing_attempts.erase(attempt_it);
    m_outgoing_conn_count++;

    AddConnection(data);

    OnReadyForFirstSend(data.conn, data.id);
}

void CConnectionHandler::OutgoingConnectionFailure(uint64_t id)
{
    assert(IsEventThread());
    connections_list_type::iterator attempt_it = m_outgoing_attempts.find(id);
    assert(attempt_it != m_outgoing_attempts.end());
    
    connection_data old_data_copy = attempt_it->second;
    m_outgoing_attempts.erase(id);

    if(old_data_copy.bev)
    {
        bufferevent_free(old_data_copy.bev);
        old_data_copy.bev = NULL;
    }

    CConnection failedConn = old_data_copy.conn;
    if(!old_data_copy.resolved_conns.empty())
    {
        failedConn = old_data_copy.resolved_conns.front();
        old_data_copy.resolved_conns.pop_front();
    }

    connection_data* newdata = NULL;
    bool should_retry = !m_shutdown && (!old_data_copy.resolved_conns.empty() || old_data_copy.retry != 0);
    if(should_retry)
    {
        newdata = CreateRetry(old_data_copy);
        if(newdata->retry > 0)
            newdata->retry--;
    }

    if(old_data_copy.conn.GetProxy().IsSet() && !old_data_copy.proxy_connected)
        OnProxyFailure(old_data_copy.conn, failedConn, id, should_retry, should_retry ? newdata->id : 0);
    else if(old_data_copy.conn.IsDNS() && failedConn.GetOptions().doResolve == CConnectionOptions::RESOLVE_ONLY)
        OnDnsFailure(old_data_copy.conn);
    else
        OnConnectionFailure(old_data_copy.conn, failedConn, id, should_retry, should_retry ? newdata->id : 0);

    if(should_retry)
        AddTimedConnect(newdata, newdata->conn.GetOptions().nRetryInterval);
    else
        ActivateConnectionsTimer();
}

void CConnectionHandler::HandleDisconnected(const connection_data& old_data_copy)
{
    bool persistent = !old_data_copy.incoming && old_data_copy.conn.GetOptions().fPersistent;
    uint64_t newid = 0;
    if(old_data_copy.incoming)
    {
        std::map<uint64_t, listener_data>::iterator it = m_binds.find(old_data_copy.listen_conn);
        if(it != m_binds.end())
        {
            it->second.incoming_connections--;
        }
        m_incoming_conn_count--;
    }
    else
    {
        if(persistent && !m_shutdown)
        {
            connection_data* newdata = CreateRetry(old_data_copy);
            newid = newdata->id;
            newdata->retry = old_data_copy.conn.GetOptions().nRetries;
            AddTimedConnect(newdata, old_data_copy.conn.GetOptions().nRetryInterval);
        }
        m_outgoing_conn_count--;
    }
    OnDisconnected(old_data_copy.conn, old_data_copy.id, persistent, newid);
    if(!persistent && !old_data_copy.incoming && !m_shutdown)
        ActivateConnectionsTimer();
}

void CConnectionHandler::AddTimedConnect(connection_data* data, int seconds)
{
    assert(IsEventThread());
    assert(!data->ev);
    if(m_shutdown)
        return;
    data->ev = event_new(m_event_base, -1, 0, timer_callbacks::retry_outgoing, data);
    timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    event_add(data->ev, &timeout);
}

connection_data* CConnectionHandler::CreateOutgoing(const CConnection& conn)
{
    assert(IsEventThread());
    uint64_t id = m_outgoing_attempt_index++;
    connection_data* data = &m_outgoing_attempts[id];
    data->handler = this;
    data->id = id;
    data->conn = conn;
    data->bev = NULL;
    data->retry = conn.GetOptions().nRetries;
    return data;
}

connection_data* CConnectionHandler::CreateRetry(const connection_data& olddata)
{
    connection_data* data = CreateOutgoing(olddata.conn);
    data->resolved_conns = olddata.resolved_conns;
    data->retry = olddata.retry;
    return data;
}

void CConnectionHandler::LookupResults(uint64_t id, std::list<CConnection>& results)
{
    assert(IsEventThread());
    connection_data& data = m_outgoing_attempts[id];
    if(data.conn.GetOptions().doResolve == CConnectionOptions::RESOLVE_ONLY)
    {
        OnDnsResponse(data.conn, id, results);
        if(data.bev)
        {
            bufferevent_free(data.bev);
            data.bev = NULL;
        }
        m_outgoing_attempts.erase(id);
        ActivateConnectionsTimer();
    }
    else
    {
        data.resolved_conns = results;
        ConnectOutgoing(&data);
    }
}

void CConnectionHandler::WriteBufferReady(connection_data* data)
{
    assert(IsEventThread());
    evbuffer* output = bufferevent_get_output(data->bev);
    size_t bufsize = evbuffer_get_length(output);
    OnWriteBufferReady(data->id, bufsize);
}

bool CConnectionHandler::ConnectOutgoing(connection_data* data)
{
    assert(IsEventThread());
    CConnectionOptions opts = data->conn.GetOptions();
    CConnectionBase baseConn = data->conn.GetProxy();
    if(!baseConn.IsSet())
        baseConn = data->conn;
    if(baseConn.IsDNS() && data->resolved_conns.empty())
    {
        unsigned short port = baseConn.GetPort();
        char portstr[NI_MAXSERV] = {};
        evutil_snprintf(portstr, sizeof(portstr), "%hu", port);
        evutil_addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = EVUTIL_AI_NUMERICSERV;
        hints.ai_flags |= opts.doResolve == CConnectionOptions::NO_RESOLVE ? EVUTIL_AI_NUMERICHOST : EVUTIL_AI_ADDRCONFIG;
        if(opts.resolveFamily == CConnectionOptions::IPV4)
            hints.ai_family = AF_INET;
        else if(opts.resolveFamily == CConnectionOptions::IPV6)
            hints.ai_family = AF_INET6;
        else
            hints.ai_family = AF_UNSPEC;

        evdns_getaddrinfo(m_dns_base, baseConn.GetHost().c_str(), portstr, &hints, dns_callbacks::dns_callback, data);
        return true;
    }

    bufferevent_options bev_opts = bufferevent_options(BEV_OPT_CLOSE_ON_FREE);
    if(m_enable_threading)
        bev_opts = bufferevent_options(bev_opts | BEV_OPT_THREADSAFE);
    data->bev = bufferevent_socket_new(m_event_base, -1, bev_opts);
    if(!data->bev)
    {
        OutgoingConnectionFailure(data->id);
        return false;
    }

    bufferevent_disable(data->bev, EV_READ | EV_WRITE);

    timeval connTimeout;
    connTimeout.tv_sec = data->conn.GetOptions().nConnTimeout;
    connTimeout.tv_usec = 0;
    bufferevent_set_timeouts(data->bev, &connTimeout, &connTimeout);

    bufferevent_setcb(data->bev, NULL, NULL, connection_callbacks::outgoing_conn, data);

    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    int socklen = sizeof(addr);
    if(baseConn.IsDNS())
    {
        if(data->resolved_conns.size())
        {
            CConnection& resolved = data->resolved_conns.front();
            resolved.GetSockAddr((sockaddr*)&addr, &socklen);
        }
        else
        {
            OutgoingConnectionFailure(data->id);
            return false;
        }
    }
    else
    {
        baseConn.GetSockAddr((sockaddr*)&addr, &socklen);
    }

    if (bufferevent_socket_connect(data->bev, (sockaddr*)&addr, socklen) != 0)
    {
        return false;
    }

    fd_type sock_fd;
    sock_fd.fd = bufferevent_getfd(data->bev);
    SetSocketOpts((sockaddr*)&addr, socklen, sock_fd);

    return true;
}

void CConnectionHandler::DisconnectNow(uint64_t id)
{
    connection_data old_data_copy;
    {
        optional_lock(m_conn_mutex, m_enable_threading);
        connections_list_type::iterator it = m_connected.find(id);
        if(it == m_connected.end())
            return;
        old_data_copy = it->second;
        m_connected.erase(it);
        bufferevent_lock(old_data_copy.bev);
    }
    bufferevent* bev = old_data_copy.bev;
    bufferevent_disable(bev, EV_READ | EV_WRITE);
    bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
    bufferevent_set_rate_limit(bev, NULL);
    bufferevent_remove_from_rate_limit_group(bev);
    bufferevent_free(bev);
    bufferevent_unlock(bev);
    if(old_data_copy.rate_cfg)
        ev_token_bucket_cfg_free(old_data_copy.rate_cfg);

    HandleDisconnected(old_data_copy);
}

void CConnectionHandler::DisconnectAfterWrite(uint64_t id)
{
    connection_data old_data_copy;
    connection_data* data_ptr;
    bufferevent* bev;
    bool erased = false;
    {
        optional_lock(m_conn_mutex, m_enable_threading);
        connections_list_type::iterator it = m_connected.find(id);
        if(it == m_connected.end())
            return;
        old_data_copy = it->second;
        data_ptr = &it->second;
        bev = old_data_copy.bev;
        bufferevent_lock(bev);
        const evbuffer *output = bufferevent_get_output(bev);
        if (!evbuffer_get_length(output))
        {
            data_ptr->bev = NULL;
            m_connected.erase(it);
            erased = true;
        }
    }
    if(erased)
    {
        bufferevent_disable(bev, EV_READ | EV_WRITE);
        bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
        bufferevent_set_rate_limit(bev, NULL);
        bufferevent_remove_from_rate_limit_group(bev);
        bufferevent_free(bev);
        bufferevent_unlock(bev);
        if(old_data_copy.rate_cfg)
            ev_token_bucket_cfg_free(old_data_copy.rate_cfg);

        HandleDisconnected(old_data_copy);
    }
    else
    {
        bufferevent_disable(bev, EV_READ);
        bufferevent_setcb(bev, NULL, bev_callbacks::close_on_finished_writecb, bev_callbacks::event_cb, data_ptr);
        bufferevent_setwatermark(bev, EV_WRITE, 0, 0);
        bufferevent_unlock(bev);
    }
}

void CConnectionHandler::BindFailure(listener_data* data)
{
    assert(IsEventThread());

    uint64_t newid = 0;
    if(data->retry != 0)
        newid = m_bind_attempt_index++;

    OnBindFailure(data->conn, data->id, data->retry, newid);

    if(data->retry != 0)
    {
        listener_data& newdata = m_bind_attempts[newid];
        newdata = *data;
        newdata.id = newid;
        if(newdata.retry > 0)
            newdata.retry--;
        timeval timeout;
        timeout.tv_sec = data->conn.GetOptions().nRetryInterval;
        timeout.tv_usec = 0;
        newdata.ev = event_new(m_event_base, -1, 0, timer_callbacks::retry_bind, &newdata);
        event_add(newdata.ev, &timeout);
    }
    m_bind_attempts.erase(data->id);
}

void CConnectionHandler::CreateListener(const CConnectionListener& listener)
{
    assert(IsEventThread());
    uint64_t id = m_bind_attempt_index++;
    listener_data& data = m_bind_attempts[id];
    data.id = id;
    data.handler = this;
    data.conn = listener;
    data.retry = listener.GetOptions().nRetries;
    data.ev = NULL;
}

bool CConnectionHandler::ConnectListener(uint64_t id)
{
    assert(IsEventThread());
    listener_data& data = m_bind_attempts[id];

    sockaddr_storage addr;
    int socklen = sizeof(addr);
    memset(&addr, 0, socklen);
    data.conn.GetSockAddr((sockaddr*)&addr, &socklen);
    evconnlistener* listen = evconnlistener_new_bind(m_event_base, NULL, NULL, LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,(sockaddr*)&addr, socklen);
    if(listen)
    {
        uint64_t newid = m_bind_index++;
        listener_data& datatmp = m_binds[newid];
        datatmp = data;
        datatmp.id = newid;
        datatmp.handler = this;
        datatmp.listener = listen;
        m_bind_attempts.erase(id);
        OnBind(datatmp.conn, id, newid);
        evconnlistener_set_cb(listen, connection_callbacks::accept_conn, &datatmp);
        evconnlistener_set_error_cb(listen, connection_callbacks::listen_error_cb);
        evconnlistener_enable(listen);
    }
    else
    {
        BindFailure(&data);
    }

    return listen != NULL;
}

void CConnectionHandler::RequestBind()
{
    assert(IsEventThread());
    if(m_bind_limit < 0 || m_binds.size() + m_bind_attempts.size() < (uint64_t)m_bind_limit)
    {
        CConnectionListener listener;
        uint64_t id = m_bind_attempt_index;
        if(OnNeedIncomingListener(listener, id))
        {
            CreateListener(listener);
            ConnectListener(id);
        }
    }
}

void CConnectionHandler::RequestOutgoing()
{
    assert(IsEventThread());
    if(m_shutdown)
        return;
    int need = m_outgoing_conn_limit < 0 ? 8 : m_outgoing_conn_limit - m_outgoing_conn_count - (int)m_outgoing_attempts.size();
    if(need > 0)
    {
        std::vector<CConnection> conns;
        if(OnNeedOutgoingConnections(conns, need))
        {
            for(std::vector<CConnection>::const_iterator it = conns.begin(); it != conns.end(); ++it)
            {
                if(it->IsSet())
                {
                    connection_data* data = CreateOutgoing(*it);
                    ConnectOutgoing(data);
                    if(!--need)
                        break;
                }
            }
        }
    }
}

int CConnectionHandler::PumpEvents(bool block)
{
    assert(IsEventThread());
    if(!m_event_base)
        return false;
    return event_base_loop(m_event_base, block ? EVLOOP_ONCE : EVLOOP_NONBLOCK);
}

void CConnectionHandler::CloseConnection(uint64_t id, bool immediately)
{
    event_payload* payload = new event_payload;
    payload->id = id;
    payload->handler = this;
    payload->immediately = immediately;

    payload->ev = event_new(m_event_base, -1, 0, connection_callbacks::force_disconnect, payload);
    event_active(payload->ev, 0, 0);
}

void CConnectionHandler::ReceiveMessages(connection_data* data)
{
    assert(IsEventThread());

    if(m_shutdown)
        return;

    assert(data);
    assert(data->bev);
    evbuffer* input = bufferevent_get_input(data->bev);

    bool fComplete = false;
    bool fBadMsgStart = false;
    uint32_t msgsize = 0;
    CNodeMessages msgs;
    msgs.first_receive = false;
    msgs.size = 0;
    const CNetworkConfig& netconfig = data->conn.GetNetConfig();
    do
    {
        msgsize = first_complete_message_size(netconfig, input, fComplete, fBadMsgStart);
        if(netconfig.message_max_size > 0 && msgsize > netconfig.message_max_size)
        {
            OnMalformedMessage(data->id);
            continue;
        }
        if(fBadMsgStart)
        {
            bufferevent_disable(data->bev, EV_READ);
            OnMalformedMessage(data->id);
            return;
        }
        if(msgsize && fComplete)
        {
            std::vector<unsigned char> msg(msgsize);
            assert(evbuffer_remove(input, &msg[0], msgsize) == (int)msgsize);
            msgs.messages.push_back(msg);
            msgs.size += msgsize;
            if(!data->received_first_message)
            {
                msgs.first_receive = true;
                data->received_first_message = true;
            }
        }
    }
    while(fComplete);

    if(msgsize)
        bufferevent_setwatermark(data->bev, EV_READ, msgsize, 0);
    else
        bufferevent_setwatermark(data->bev, EV_READ, 20, 0);

    if(msgs.size)
        OnReceiveMessages(data->id, msgs);
}

int CConnectionHandler::PauseRecv(uint64_t id)
{
    int refcount;
    bufferevent* bev = NULL;
    {
        optional_lock(m_conn_mutex, m_enable_threading);
        connections_list_type::iterator conn = m_connected.find(id);
        if(conn == m_connected.end())
            return -1;
        if(conn->second.disconnecting)
            return -2;
        refcount = ++conn->second.pause_recv;
        if(refcount > 1)
            return refcount;
        bev = conn->second.bev;
        bufferevent_lock(bev);
    }
    int ret = bufferevent_disable(bev, EV_READ);
    bufferevent_unlock(bev);
    return ret;
}

int CConnectionHandler::UnpauseRecv(uint64_t id)
{
    bufferevent* bev = NULL;
    int refcount;
    {
        optional_lock(m_conn_mutex, m_enable_threading);
        connections_list_type::iterator conn = m_connected.find(id);
        if(conn == m_connected.end())
            return -1;
        if(conn->second.disconnecting)
            return -2;
        assert(conn->second.pause_recv >= 0);
        if(conn->second.pause_recv == 0)
            return -3;
        refcount = --conn->second.pause_recv;
        if(refcount)
            return refcount;
        bev = conn->second.bev;
        bufferevent_lock(bev);
    }
    int ret = bufferevent_enable(bev, EV_READ);
    bufferevent_unlock(bev);
    return ret;
}

bool CConnectionHandler::SendMsg(uint64_t id, const unsigned char* data, size_t size)
{
    bufferevent* bev = NULL;
    size_t max_write_buffer;
    connection_data* data_ptr = NULL;
    {
        optional_lock(m_conn_mutex, m_enable_threading);
        connections_list_type::iterator conn_it = m_connected.find(id);
        if(conn_it == m_connected.end())
            return false;
        data_ptr = &conn_it->second;
        if(data_ptr->disconnecting)
            return false;
        bev = data_ptr->bev;
        max_write_buffer = data_ptr->conn.GetOptions().nMaxSendBuffer;
        bufferevent_lock(bev);
    }

    evbuffer *output = bufferevent_get_output(bev);
    size_t oldsize = evbuffer_get_length(output);
    assert(evbuffer_add(output, data, size) == 0);
    size_t newsize = evbuffer_get_length(output);
    if(oldsize < max_write_buffer && newsize >= max_write_buffer)
    {
        OnWriteBufferFull(id, newsize);
        bufferevent_setcb(bev, bev_callbacks::read_cb, bev_callbacks::reenable_read_cb, bev_callbacks::event_cb, data_ptr);
    }
    bufferevent_unlock(bev);
    return true;

}

bool CConnectionHandler::SetRateLimit(uint64_t id, const CRateLimit& limit)
{
    ev_token_bucket_cfg* cfg = ev_token_bucket_cfg_new(limit.nMaxReadRate, limit.nMaxBurstRead, limit.nMaxWriteRate, limit.nMaxBurstWrite, NULL);
    ev_token_bucket_cfg* old_cfg = NULL;
    bufferevent* bev = NULL;
    connection_data* data_ptr = NULL;
    {
        optional_lock(m_conn_mutex, m_enable_threading);
        connections_list_type::iterator conn_it = m_connected.find(id);
        if(conn_it == m_connected.end())
            return false;
        data_ptr = &conn_it->second;
        if(data_ptr->disconnecting)
            return false;
        bev = data_ptr->bev;
        old_cfg = data_ptr->rate_cfg;
        data_ptr->rate_cfg = cfg;
        bufferevent_lock(bev);
    }
    bufferevent_set_rate_limit(bev, data_ptr->rate_cfg);
    bufferevent_unlock(bev);
    if(old_cfg != NULL)
        ev_token_bucket_cfg_free(old_cfg);
    return true;
}

void CConnectionHandler::SetGroupRateLimit(const CRateLimit& limit, ev_token_bucket_cfg*& cfg, bufferevent_rate_limit_group* group)
{
    ev_token_bucket_cfg* temp_cfg = ev_token_bucket_cfg_new(limit.nMaxReadRate, limit.nMaxBurstRead, limit.nMaxWriteRate, limit.nMaxBurstWrite, NULL);
    if(group)
        assert(bufferevent_rate_limit_group_set_cfg(group, temp_cfg) == 0);
    if(cfg)
        ev_token_bucket_cfg_free(cfg);
    cfg = temp_cfg;
}

void CConnectionHandler::SetIncomingRateLimit(const CRateLimit& limit)
{
    optional_lock(m_rate_mutex, m_enable_threading);
    SetGroupRateLimit(limit, m_incoming_rate_cfg, m_incoming_rate_limit);
}

void CConnectionHandler::SetOutgoingRateLimit(const CRateLimit& limit)
{
    optional_lock(m_rate_mutex, m_enable_threading);
    SetGroupRateLimit(limit, m_outgoing_rate_cfg, m_outgoing_rate_limit);
}


