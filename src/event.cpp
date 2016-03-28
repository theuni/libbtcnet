// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "event.h"
#include "eventtypes.h"

#include <event2/event.h>
#include <assert.h>
#include <stdio.h>

CEvent::CEvent() : m_event(nullptr)
{
}

CEvent::CEvent(const event_type<event_base>& base, evutil_socket_t sock, short flags, std::function<void()>&& func)
{
    reset(base, sock, flags, std::move(func));
}

CEvent::~CEvent()
{
    free();
}

CEvent::operator bool() const
{
    return m_event.operator bool();
}

void CEvent::free()
{
    m_event.free();
}

void CEvent::reset(const event_type<event_base>& base, evutil_socket_t sock, short flags, std::function<void()>&& func)
{
    assert(base);
    m_func = std::move(func);
    m_event = event_type<event>(event_new(base, sock, flags, callback, this));
    assert(m_event);
}

void CEvent::del()
{
    if (m_event)
        event_del(m_event);
}

void CEvent::active()
{
    if (m_event)
        event_active(m_event, EV_TIMEOUT, 0);
}

void CEvent::add(const timeval* tv)
{
    if (m_event)
        event_add(m_event, tv);
}

void CEvent::priority_set(int priority)
{
    if (m_event)
        event_priority_set(m_event, priority);
}

void CEvent::callback(evutil_socket_t, short /*unused*/, void* ctx)
{
    assert(ctx != nullptr);
    CEvent* obj = static_cast<CEvent*>(ctx);
    obj->m_func();
}
