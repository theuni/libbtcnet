// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_CALLBACKS_H
#define BITCOIN_NET_CALLBACKS_H

#include <event2/util.h>

struct bufferevent;

struct bev_callbacks
{
    static void read_cb(bufferevent *bev, void *ctx);
    static void event_cb(bufferevent *bev, short events, void *ctx);
    static void reenable_read_cb(bufferevent *bev, void *ctx);
    static void close_on_finished_writecb(bufferevent *bev, void *ctx);
    static void first_send_cb(bufferevent *bev, void *ctx);
    static void first_recv_cb(bufferevent *bev, void *ctx);
};

struct buf_callbacks
{
    static void read_data(struct evbuffer *buffer, const struct evbuffer_cb_info *info, void *ctx);
    static void wrote_data(struct evbuffer *buffer, const struct evbuffer_cb_info *info, void *ctx);
};

struct dns_callbacks
{
    static void dns_callback(int result, struct evutil_addrinfo *res, void *ctx);
};

struct timer_callbacks
{
    static void retry_bind(evutil_socket_t, short, void *ctx);
    static void retry_outgoing(evutil_socket_t, short, void *ctx);
    static void request_connections(evutil_socket_t, short, void *ctx);
};

#endif // BITCOIN_NET_CALLBACKS_H
