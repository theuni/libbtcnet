// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_EVENTTYPES_H
#define LIBBTCNET_SRC_EVENTTYPES_H

#include <cstddef>
#include <assert.h>

struct event_config;
struct event;
struct ev_token_bucket_cfg;
struct bufferevent_rate_limit_group;
struct event_base;
struct evdns_base;
struct bufferevent;
struct evconnlistener;
struct evdns_getaddrinfo_request;

template <typename T>
class event_type
{
public:
    event_type()
        : m_obj(nullptr)
    {
    }

    explicit event_type(T* obj)
        : m_obj(obj)
    {
        assert(m_obj != nullptr);
    }

    explicit event_type(std::nullptr_t)
        : m_obj(nullptr)
    {
    }

    event_type(event_type&& rhs) noexcept
        : m_obj(rhs.m_obj)
    {
        rhs.m_obj = nullptr;
    }

    ~event_type()
    {
        free();
    }

    static void obj_free(T* tofree);

    inline void free()
    {
        if (m_obj != nullptr)
            event_type::obj_free(m_obj);
        m_obj = nullptr;
    }

    inline void reset(T* rhs)
    {
        m_obj = rhs;
    }

    inline void swap(event_type& rhs)
    {
        T* temp = m_obj;
        m_obj = rhs.m_obj;
        rhs.m_obj = temp;
    }

    inline operator T*() const
    {
        assert(m_obj != nullptr);
        return m_obj;
    }

    inline explicit operator bool() const
    {
        return m_obj != nullptr;
    }

    inline bool operator!() const
    {
        return m_obj == nullptr;
    }

    template <typename V>
    inline bool operator==(const V& rhs) const
    {
        return m_obj == rhs;
    }

    template <typename V>
    inline bool operator!=(const V& rhs) const
    {
        return m_obj != rhs;
    }

    inline event_type& operator=(T* rhs)
    {
        assert(rhs);
        free();
        m_obj = rhs;
        return *this;
    }

    inline event_type& operator=(std::nullptr_t)
    {
        free();
        return *this;
    }

    inline event_type& operator=(event_type&& rhs) noexcept
    {
        free();
        m_obj = rhs.m_obj;
        rhs.reset(nullptr);
        return *this;
    }
    event_type& operator=(const event_type&) = delete;
    event_type(const event_type&) = delete;

private:
    T* m_obj;
};
#endif
