// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "connectionbase.h"
#include "message.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/event.h"
#include "eventtypes.h"
#include <assert.h>
#include "logger.h"
#include <stdio.h>

struct BufferEventLocker {
    explicit BufferEventLocker(bufferevent* bev) : m_bev(bev)
    {
        bufferevent_lock(m_bev);
    }
    ~BufferEventLocker()
    {
        bufferevent_unlock(m_bev);
    }

private:
    bufferevent* m_bev;
};

ConnectionBase::ConnectionBase(CConnectionHandlerInt& handler, CConnection&& conn, ConnID id)
    : m_handler(handler), m_event_base(handler.GetEventBase()), m_connection(std::move(conn)), m_id(id), m_reconnect_func(m_event_base, 0, std::bind(&ConnectionBase::Connect, this)), m_disconnect_func(m_event_base, 0, std::bind(&ConnectionBase::DisconnectInt, this, 0)), m_disconnect_wait_func(m_event_base, 0, std::bind(&ConnectionBase::DisconnectWhenFinishedInt, this)), m_check_write_buffer_func(m_event_base, 0, std::bind(&ConnectionBase::CheckWriteBufferInt, this)), m_ping_timeout_func(m_event_base, 0, std::bind(&ConnectionBase::PingTimeoutInt, this))
{
}

ConnectionBase::~ConnectionBase() = default;

void ConnectionBase::Disconnect()
{
    m_disconnect_func.active();
}

void ConnectionBase::DisconnectWhenFinished()
{
    m_disconnect_wait_func.active();
}

void ConnectionBase::ResetPingTimeout(int seconds)
{
    if (seconds != 0) {
        timeval timeout = {seconds, 0};
        m_ping_timeout_func.add(timeout);
    } else
        m_ping_timeout_func.del();
}

void ConnectionBase::PauseRecv()
{
    assert(m_bev);
    BufferEventLocker lock(m_bev);
    bufferevent_disable(m_bev, EV_READ);
}

void ConnectionBase::UnpauseRecv()
{
    assert(m_bev);
    BufferEventLocker lock(m_bev);
    bufferevent_enable(m_bev, EV_READ);
}

void ConnectionBase::Enable()
{
    bufferevent_enable(m_bev, EV_READ | EV_WRITE);
}

void ConnectionBase::Retry(ConnID newId)
{
    m_bev.free();
    m_rate_cfg.free();
    m_bytes_read = 0;
    m_bytes_written = 0;

    m_id = newId;
    DEBUG_PRINT(LOGVERBOSE, "id:", m_id, "queuing reconnect");
    timeval timeout{m_connection.GetOptions().nRetryInterval, 0};
    m_reconnect_func.add(timeout);
}

void ConnectionBase::SetRateLimit(const CRateLimit& limit)
{
    assert(m_bev != nullptr);

    event_type<ev_token_bucket_cfg> rate_cfg_new(ev_token_bucket_cfg_new(limit.nMaxReadRate, limit.nMaxBurstRead, limit.nMaxWriteRate, limit.nMaxBurstWrite, nullptr));
    BufferEventLocker lock(m_bev);
    bufferevent_set_rate_limit(m_bev, rate_cfg_new);
    m_rate_cfg.swap(rate_cfg_new);
}

void ConnectionBase::OnConnectionFailure(ConnectionFailureType type, int error, CConnection failed, bool retry)
{
    DEBUG_PRINT(LOGINFO, "id:", m_id, "connection attempt failed");
    m_handler.OnConnectionFailure(m_id, type, error, std::move(failed), retry);
}

const CConnection& ConnectionBase::GetBaseConnection() const
{
    return m_connection;
}

void ConnectionBase::DisconnectInt(int /*reason*/)
{
    DEBUG_PRINT(LOGINFO, "id:", m_id, "disconnecting");
    m_disconnect_func.del();
    m_reconnect_func.del();
    m_disconnect_wait_func.del();
    m_check_write_buffer_func.del();
    m_ping_timeout_func.del();
    {
        BufferEventLocker lock(m_bev);
        bufferevent_disable(m_bev, EV_READ | EV_WRITE);
        bufferevent_setcb(m_bev, nullptr, nullptr, nullptr, nullptr);
    }
    bool reconnect = m_connection.GetOptions().fPersistent && this->IsOutgoing();
    m_handler.OnDisconnected(m_id, reconnect);
}

void ConnectionBase::DisconnectWhenFinishedInt()
{
    assert(m_bev);
    bool now;
    {
        BufferEventLocker lock(m_bev);
        evbuffer* output = bufferevent_get_output(m_bev);
        if (evbuffer_get_length(output) != 0u) {
            DEBUG_PRINT(LOGINFO, "id:", m_id, "disconnecting when finished");
            bufferevent_disable(m_bev, EV_READ);
            bufferevent_setwatermark(m_bev, EV_WRITE, 0, 0);
            bufferevent_setcb(m_bev, nullptr, close_on_finished_writecb, event_cb, this);
            now = false;
        } else {
            bufferevent_disable(m_bev, EV_READ | EV_WRITE);
            bufferevent_setcb(m_bev, nullptr, nullptr, nullptr, nullptr);
            now = true;
        }
    }
    if (now)
        DisconnectInt(0);
}

void ConnectionBase::SetRateLimitGroup(event_type<bufferevent_rate_limit_group>& group)
{
    bufferevent_add_to_rate_limit_group(m_bev, group);
}

void ConnectionBase::InitConnection()
{
    assert(m_bev);
    const CConnection& conn = m_connection;
    const CConnectionOptions& opts = conn.GetOptions();

    evutil_socket_t sock = bufferevent_getfd(m_bev);
    evutil_make_socket_nonblocking(sock);

    bufferevent_disable(m_bev, EV_READ | EV_WRITE);

    timeval initialTimeout;
    initialTimeout.tv_sec = opts.nInitialTimeout;
    initialTimeout.tv_usec = 0;
    bufferevent_set_timeouts(m_bev, &initialTimeout, &initialTimeout);

    bufferevent_setwatermark(m_bev, EV_READ, 0, 0);
    bufferevent_setwatermark(m_bev, EV_WRITE, opts.nMaxSendBuffer, 0);

    // Don't set the regular callbacks yet. Before any data is sent/received,
    // The initial timeout is in effect. first_read_cb/first_write_cb
    // (whichever is hit first) will set the non-initial-timeout callbacks.
    bufferevent_setcb(m_bev, first_read_cb, first_write_cb, event_cb, this);

    // Add an additional set of callbacks responsible for reporting _all_
    // socket reads/writes, as opposed to the bufferevent read callback, which
    // has a watermark set.
    evbuffer* input = bufferevent_get_input(m_bev);
    evbuffer* output = bufferevent_get_output(m_bev);
    evbuffer_add_cb(input, read_data, this);
    evbuffer_add_cb(output, wrote_data, this);
}

void ConnectionBase::OnOutgoingConnected(event_type<bufferevent>&& bev, CConnection resolved)
{
    m_bev = std::move(bev);
    DEBUG_PRINT(LOGINFO, "id:", m_id, "outgoing connection: initial:", m_connection.GetHost(), "resolved:", resolved.GetHost());
    InitConnection();

    m_handler.OnOutgoingConnected(m_id, m_connection, std::move(resolved));
    // Don't do anything after OnOutgoingConnected. It may delete *this.
}

void ConnectionBase::OnIncomingConnected(event_type<bufferevent>&& bev, sockaddr* addr, int addrsize)
{
    m_bev = std::move(bev);
    CConnection resolved(m_connection.GetOptions(), m_connection.GetNetConfig(), addr, addrsize);
    DEBUG_PRINT(LOGINFO, "id:", m_id, "incoming connection. bound to:", m_connection.GetHost(), "incoming:", resolved.GetHost());
    InitConnection();

    m_handler.OnIncomingConnected(m_id, m_connection, std::move(resolved));
    // Don't do anything after OnIncomingConnected. It may delete *this.
}

// May not be on main thread!
bool ConnectionBase::Write(const unsigned char* data, size_t size)
{
    bool ret = bufferevent_write(m_bev, data, size) == 0;
    if (ret)
        m_check_write_buffer_func.active();
    return ret;
}

void ConnectionBase::CheckWriteBufferInt()
{
    DEBUG_PRINT(LOGVERBOSE, "id:", m_id, "Checking write buffer");
    assert(m_bev);
    bool full = false;
    size_t buflen;
    int maxsend = m_connection.GetOptions().nMaxSendBuffer;
    {
        BufferEventLocker lock(m_bev);
        evbuffer* output = bufferevent_get_output(m_bev);
        buflen = evbuffer_get_length(output);
        if (static_cast<int>(buflen) >= maxsend) {
            full = true;
            bufferevent_setcb(m_bev, read_cb, write_cb, event_cb, this);
        }
    }
    if (full)
        m_handler.OnWriteBufferFull(m_id, buflen);
}

void ConnectionBase::PingTimeoutInt()
{
    m_handler.OnPingTimeout(m_id);
}

void ConnectionBase::first_read_cb(bufferevent* bev, void* ctx)
{
    assert(ctx);
    ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
    timeval recvTimeout = {base->m_connection.GetOptions().nRecvTimeout, 0};
    timeval sendTimeout = {base->m_connection.GetOptions().nSendTimeout, 0};
    bufferevent_set_timeouts(base->m_bev, &recvTimeout, &sendTimeout);
    bufferevent_setcb(bev, read_cb, nullptr, event_cb, ctx);
    read_cb(bev, ctx);
}

void ConnectionBase::first_write_cb(bufferevent* bev, void* ctx)
{
    assert(ctx);
    ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
    timeval recvTimeout = {base->m_connection.GetOptions().nRecvTimeout, 0};
    timeval sendTimeout = {base->m_connection.GetOptions().nSendTimeout, 0};
    bufferevent_set_timeouts(base->m_bev, &recvTimeout, &sendTimeout);
    bufferevent_setcb(bev, read_cb, nullptr, event_cb, ctx);
}

void ConnectionBase::read_data(struct evbuffer* /*unused*/, const struct evbuffer_cb_info* info, void* ctx)
{
    assert(ctx);
    if (info->n_added != 0u) {
        ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
        base->m_bytes_read += info->n_added;
        base->m_handler.m_interface.OnBytesRead(base->m_id, info->n_added, base->m_bytes_read);
        DEBUG_PRINT(LOGALL, "id:", base->m_id, "Read:", info->n_added, "bytes. Total:", base->m_bytes_read);
    }
}

void ConnectionBase::wrote_data(struct evbuffer* /*unused*/, const struct evbuffer_cb_info* info, void* ctx)
{
    assert(ctx);
    if (info->n_deleted != 0u) {
        ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
        base->m_bytes_written += info->n_deleted;
        base->m_handler.m_interface.OnBytesWritten(base->m_id, info->n_deleted, base->m_bytes_written);
        DEBUG_PRINT(LOGALL, "id:", base->m_id, "Wrote:", info->n_deleted, "bytes. Total:", base->m_bytes_written);
    }
}

void ConnectionBase::event_cb(bufferevent* /*unused*/, short type, void* ctx)
{
    assert(ctx);
    ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
    if (((type & BEV_EVENT_TIMEOUT) != 0) ||
        ((type & BEV_EVENT_EOF) != 0) ||
        ((type & BEV_EVENT_ERROR) != 0))
        base->DisconnectInt(0);
}

void ConnectionBase::read_cb(bufferevent* bev, void* ctx)
{
    assert(ctx);
    assert(bev);
    ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
    const CNetworkConfig& netconfig = base->m_connection.GetNetConfig();
    std::list<std::vector<unsigned char> > msgs;
    size_t totalsize = 0;
    bool fTooBig = false;
    bool fBadMsgStart = false;
    {
        BufferEventLocker lock(bev);
        evbuffer* input = bufferevent_get_input(bev);

        uint64_t msgsize = 0;
        bool fComplete = false;
        do {
            msgsize = first_complete_message_size(netconfig, input, fComplete, fBadMsgStart);

            if (netconfig.message_max_size > 0 && msgsize > netconfig.message_max_size) {
                fTooBig = true;
                break;
            } else if (fBadMsgStart) {
                break;
            } else if ((msgsize != 0u) && fComplete) {
                msgs.emplace_back(msgsize, 0);
                evbuffer_remove(input, msgs.back().data(), msgsize);
                totalsize += msgsize;
            }
        } while (fComplete);

        if ((msgsize != 0u) && !fBadMsgStart && !fTooBig) {
            size_t buflen = evbuffer_get_length(input);
            if (buflen < msgsize + netconfig.header_size)
                evbuffer_expand(input, msgsize + netconfig.header_size - buflen);
            bufferevent_setwatermark(bev, EV_READ, msgsize, 0);
            DEBUG_PRINT(LOGVERBOSE, "id:", base->m_id, "watermark set to", msgsize);
        } else if (!fBadMsgStart && !fTooBig) {
            size_t watermark = netconfig.header_msg_size_offset + netconfig.header_msg_size_size;
            bufferevent_setwatermark(bev, EV_READ, watermark, 0);
            DEBUG_PRINT(LOGVERBOSE, "id:", base->m_id, "watermark set to", watermark);
        }
    }

    if (fBadMsgStart) {
        DEBUG_PRINT(LOGWARN, "id:", base->m_id, "Received a bad message start");
        base->DisconnectInt(0);
    } else if (fTooBig) {
        DEBUG_PRINT(LOGWARN, "id:", base->m_id, "Received an oversized message");
        base->DisconnectInt(0);
    } else if (totalsize != 0u) {
        DEBUG_PRINT(LOGINFO, "id:", base->m_id, "Received", msgs.size(), "messages");
        base->m_handler.OnReceiveMessages(base->m_id, std::move(msgs), totalsize);
    }
}

void ConnectionBase::write_cb(bufferevent* bev, void* ctx)
{
    assert(ctx);
    ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
    evbuffer* output = bufferevent_get_output(bev);
    size_t buflen = evbuffer_get_length(output);
    bufferevent_setcb(bev, read_cb, nullptr, event_cb, ctx);
    base->m_handler.OnWriteBufferReady(base->m_id, buflen);
}

void ConnectionBase::close_on_finished_writecb(bufferevent* /*unused*/, void* ctx)
{
    assert(ctx);
    ConnectionBase* base = static_cast<ConnectionBase*>(ctx);
    base->DisconnectInt(0);
}
