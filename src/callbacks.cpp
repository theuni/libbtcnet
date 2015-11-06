#include "callbacks.h"

#include "connection_data.h"
#include "libbtcnet/handler.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event.h>

#include <stddef.h>
#include <assert.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#endif

static void set_default_timeouts(connection_data* data)
{
    timeval recvTimeout;
    recvTimeout.tv_sec = data->conn.GetOptions().nRecvTimeout;
    recvTimeout.tv_usec = 0;

    timeval sendTimeout;
    sendTimeout.tv_sec = data->conn.GetOptions().nSendTimeout;
    sendTimeout.tv_usec = 0;

    bufferevent_set_timeouts(data->bev, &recvTimeout, &sendTimeout);
}

void bev_callbacks::read_cb(bufferevent *, void *ctx)
{
    if(ctx)
    {
        connection_data* data = static_cast<connection_data*>(ctx);
        data->handler->ReceiveMessages(data);
    }
}

void bev_callbacks::reenable_read_cb(bufferevent *bev, void *ctx)
{
    if(ctx)
    {
        connection_data* data = static_cast<connection_data*>(ctx);
        bufferevent_setcb(bev, read_cb, NULL, event_cb, ctx);
        data->handler->WriteBufferReady(data);
    }
}

void bev_callbacks::event_cb(bufferevent *, short type, void *ctx)
{
    if(ctx)
    {
        connection_data* data = static_cast<connection_data*>(ctx);
        if (type & BEV_EVENT_TIMEOUT )
        {
            data->handler->DisconnectNow(data->id);
        }
        else if (type & BEV_EVENT_EOF )
        {
            data->handler->DisconnectNow(data->id);
        }
        else if (type & BEV_EVENT_ERROR)
        {
            data->handler->DisconnectNow(data->id);
        }
    }
}

void bev_callbacks::first_recv_cb(bufferevent *bev, void *ctx)
{
    connection_data* data = static_cast<connection_data*>(ctx);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, data);
    set_default_timeouts(data);
    read_cb(bev, ctx);
}

void bev_callbacks::first_send_cb(bufferevent *bev, void *ctx)
{
    connection_data* data = static_cast<connection_data*>(ctx);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, data);
    set_default_timeouts(data);
}

void bev_callbacks::close_on_finished_writecb(bufferevent *bev, void *ctx)
{
    connection_data* data = static_cast<connection_data*>(ctx);
    evbuffer *output = bufferevent_get_output(bev);
    if (!evbuffer_get_length(output))
    {
        data->handler->DisconnectNow(data->id);
    }
}

void buf_callbacks::read_data(struct evbuffer *buffer, const struct evbuffer_cb_info *info, void *ctx)
{
    if(info->n_added)
    {
        connection_data* data = static_cast<connection_data*>(ctx);
        data->handler->BytesRead(data, info->orig_size, info->n_added);
    }
}

void buf_callbacks::wrote_data(struct evbuffer *buffer, const struct evbuffer_cb_info *info, void *ctx)
{
    if(info->n_deleted)
    {
        connection_data* data = static_cast<connection_data*>(ctx);
        data->handler->BytesWritten(data, info->orig_size, info->n_deleted);
    }
}

void dns_callbacks::dns_callback(int result, evutil_addrinfo *ai, void *ctx)
{
    connection_data* data = static_cast<connection_data*>(ctx);
    if(result != 0 || !ai)
    {
        data->handler->OutgoingConnectionFailure(data->id);
        return;
    }

    std::list<CConnection> results;
    evutil_addrinfo* ai_iter = ai;
    const CConnectionOptions& opts = data->conn.GetOptions();
    const CNetworkConfig& netConfig = data->conn.GetNetConfig();
    const CProxy& proxy = data->conn.GetProxy();
    while(ai_iter && (opts.nMaxLookupResults <= 0 || results.size() < (size_t)opts.nMaxLookupResults))
    {
        if( (opts.resolveFamily == CConnectionOptions::IPV4 && ai_iter->ai_family == AF_INET)  ||
            (opts.resolveFamily == CConnectionOptions::IPV6 && ai_iter->ai_family == AF_INET6) ||
            (opts.resolveFamily == CConnectionOptions::UNSPEC))
        {
            results.push_back(CConnection(ai_iter->ai_addr, ai_iter->ai_addrlen, opts, netConfig, proxy));
        }
        ai_iter = ai_iter->ai_next;
    }
    data->handler->LookupResults(data->id, results);
    evutil_freeaddrinfo(ai);
}

void timer_callbacks::retry_bind(evutil_socket_t, short, void *ctx)
{
    listener_data* data = static_cast<listener_data*>(ctx);
    event_free(data->ev);
    data->ev = NULL;
    data->handler->ConnectListener(data->id);
}

void timer_callbacks::request_connections(evutil_socket_t, short, void *ctx)
{
    CConnectionHandler* handler = static_cast<CConnectionHandler*>(ctx);
    handler->RequestBind();
    handler->RequestOutgoing();
}

void timer_callbacks::retry_outgoing(evutil_socket_t, short, void *ctx)
{
    assert(ctx);
    connection_data* data = static_cast<connection_data*>(ctx);
    event_free(data->ev);
    data->ev = NULL;
    data->handler->ConnectOutgoing(data);
}
