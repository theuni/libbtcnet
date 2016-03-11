// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_RESOLVE_H
#define LIBBTCNET_SRC_RESOLVE_H

#include "eventtypes.h"
#include <event2/util.h>

struct evdns_base;
struct evdns_getaddrinfo_request;
class CDNSResponse
{
public:
    class iterator
    {
    public:
        iterator();
        explicit iterator(evutil_addrinfo* ai);
        iterator& operator++();
        iterator operator++(int);
        bool operator==(const iterator& rhs) const;
        bool operator!=(const iterator& rhs) const;
        evutil_addrinfo* operator->() const;

    private:
        evutil_addrinfo* m_ai;
    };

    CDNSResponse();
    CDNSResponse(CDNSResponse&& rhs) noexcept;
    CDNSResponse(const CDNSResponse&) = delete;
    CDNSResponse& operator=(const CDNSResponse&) = delete;
    CDNSResponse& operator=(CDNSResponse&& rhs) noexcept;
    explicit CDNSResponse(evutil_addrinfo* ai);
    ~CDNSResponse();
    void clear();
    bool empty() const;
    CDNSResponse::iterator begin() const;
    CDNSResponse::iterator end() const;

private:
    evutil_addrinfo* m_ai;
};

class CDNSResolve
{
protected:
    virtual ~CDNSResolve();
    virtual void OnResolveSuccess(CDNSResponse&& response) = 0;
    virtual void OnResolveFailure(int result) = 0;
    event_type<evdns_getaddrinfo_request> Resolve(const event_type<evdns_base>& dns_base, const char* host, const char* port, const evutil_addrinfo* hints);

private:
    static void dns_callback(int result, evutil_addrinfo* ai, void* ctx);
};

#endif // LIBBTCNET_SRC_RESOLVE_H
