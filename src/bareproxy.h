// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_BAREPROXY_H
#define LIBBTCNET_SRC_BAREPROXY_H

#include "eventtypes.h"

struct event_base;
struct bufferevent;
struct evbuffer;

class CConnection;
class CProxyAuth;

template <typename T>
class event_type;

class CBareProxy
{
public:
    explicit CBareProxy(const CConnection& conn);
    void InitProxy(event_type<bufferevent>&& bev);
    virtual ~CBareProxy();
    virtual void OnProxyFailure(int error) = 0;
    virtual void OnProxySuccess(event_type<bufferevent>&& bev, CConnection resolved) = 0;

private:
    void ProxyFailure(int error);
    static void proxy_conn_event(bufferevent* bev, short type, void* ctx);
    static bool write_auth(bufferevent* bev, const CProxyAuth& auth);
    static bool writeproto(bufferevent* bev, const CConnection& conn);
    static void receive_init(bufferevent* bev, void* ctx);
    static void check_auth_response(bufferevent* bev, void* ctx);
    static void read_final(bufferevent* bev, void* ctx);
    static void event_cb(bufferevent* /*unused*/, short events, void* ctx);

    const CConnection& m_proxy_connection;
    event_type<bufferevent> m_bev;
};

#endif // LIBBTCNET_SRC_BAREPROXY_H
