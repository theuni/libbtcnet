// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_EVENT_H
#define BTCNET_EVENT_H

#include <event2/util.h>
#include <functional>
#include "eventtypes.h"

struct event;
struct event_base;

class CEvent
{
public:
    CEvent(const event_type<event_base>& base, short flags, std::function<void()>&& func);
    CEvent();
    ~CEvent();

    void free();
    void del();
    void active();
    void add(const timeval& tv);

    void reset(const event_type<event_base>& base, short flags, std::function<void()>&& func);
    operator bool() const;

private:
    static void callback(evutil_socket_t, short, void* ctx);
    std::function<void()> m_func;
    event_type<event> m_event;

    CEvent(CEvent&& rhs) = delete;
    CEvent(const CEvent& rhs) = delete;
    CEvent& operator=(const CEvent& rhs) = delete;
    CEvent& operator=(CEvent&& rhs) = delete;
};

#endif
