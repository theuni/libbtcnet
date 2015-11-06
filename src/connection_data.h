// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_CONNECTION_DATA_H
#define BITCOIN_NET_CONNECTION_DATA_H

#include "libbtcnet/connection.h"

#include <stdint.h>
#include <list>

/* TODO: This needs tons of cleanup and documentation */

class CConnectionHandler;
struct bufferevent;
struct event;
struct evconnlistener;
struct ev_token_bucket_cfg;

struct listener_data
{
    listener_data()
    : handler(NULL)
    , listener(NULL)
    , ev(NULL)
    , id(0)
    , retry(0)
    , incoming_connections(0){}

    CConnectionHandler* handler;
    evconnlistener* listener;
    CConnectionListener conn;
    event* ev;
    uint64_t id;
    int retry;
    int incoming_connections;
};

struct connection_data
{
    connection_data()
    : handler(NULL)
    , bev(NULL)
    , ev(NULL)
    , incoming(false)
    , disconnecting(false)
    , retry(0)
    , pause_recv(false)
    , id(0)
    , listen_conn(0)
    , received_first_message(false)
    , rate_cfg(NULL)
    , proxy_connected(false)
    , bytes_written(0)
    , bytes_read(0)
    {}

    CConnectionHandler* handler;
    bufferevent* bev;
    event* ev;
    CConnection conn;
    bool incoming;
    bool disconnecting;
    int retry;
    int pause_recv;
    uint64_t id;
    uint64_t listen_conn;
    std::list<CConnection> resolved_conns;
    bool received_first_message;
    ev_token_bucket_cfg* rate_cfg;
    bool proxy_connected;
    size_t bytes_written;
    size_t bytes_read;
};

#endif // BITCOIN_NET_CONNECTION_DATA_H
