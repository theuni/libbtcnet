// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "eventtypes.h"

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>

template <>
void event_type<event_config>::obj_free(event_config* tofree)
{
    event_config_free(tofree);
}

template <>
void event_type<event>::obj_free(event* tofree)
{
    event_free(tofree);
}

template <>
void event_type<ev_token_bucket_cfg>::obj_free(ev_token_bucket_cfg* tofree)
{
    ev_token_bucket_cfg_free(tofree);
}

template <>
void event_type<bufferevent_rate_limit_group>::obj_free(bufferevent_rate_limit_group* tofree)
{
    bufferevent_rate_limit_group_free(tofree);
}

template <>
void event_type<event_base>::obj_free(event_base* tofree)
{
    event_base_free(tofree);
}

template <>
void event_type<evdns_base>::obj_free(evdns_base* tofree)
{
    evdns_base_free(tofree, 0);
}

template <>
void event_type<bufferevent>::obj_free(bufferevent* tofree)
{
    bufferevent_set_rate_limit(tofree, nullptr);
    bufferevent_remove_from_rate_limit_group(tofree);
    bufferevent_free(tofree);
}

template <>
void event_type<evconnlistener>::obj_free(evconnlistener* tofree)
{
    evconnlistener_free(tofree);
}

template <>
void event_type<evdns_getaddrinfo_request>::obj_free(evdns_getaddrinfo_request* tofree)
{
    evdns_getaddrinfo_cancel(tofree);
}
