// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "resolve.h"
#include "eventtypes.h"

#include <event2/dns.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#endif

#include <string.h>
#include <assert.h>

CDNSResponse::iterator::iterator() : m_ai(nullptr)
{
}

CDNSResponse::iterator::iterator(evutil_addrinfo* ai) : m_ai(ai)
{
}

CDNSResponse::iterator& CDNSResponse::iterator::operator++()
{
    assert(m_ai != nullptr);
    m_ai = m_ai->ai_next;
    return *this;
}

CDNSResponse::iterator CDNSResponse::iterator::operator++(int)
{
    assert(m_ai != nullptr);
    iterator ret = iterator(m_ai);
    m_ai = m_ai->ai_next;
    return ret;
}

evutil_addrinfo* CDNSResponse::iterator::operator->() const
{
    assert(m_ai != nullptr);
    return m_ai;
}

bool CDNSResponse::iterator::operator==(const iterator& rhs) const
{
    return m_ai == rhs.m_ai;
}

bool CDNSResponse::iterator::operator!=(const iterator& rhs) const
{
    return !operator==(rhs);
}

CDNSResponse::CDNSResponse() : m_ai(nullptr)
{
}

CDNSResponse::CDNSResponse(evutil_addrinfo* ai) : m_ai(ai)
{
}

CDNSResponse::~CDNSResponse()
{
    clear();
}

CDNSResponse::iterator CDNSResponse::begin() const
{
    return CDNSResponse::iterator(m_ai);
}

CDNSResponse::iterator CDNSResponse::end() const
{
    return CDNSResponse::iterator(nullptr);
}

void CDNSResponse::clear()
{
    if (m_ai != nullptr)
        evutil_freeaddrinfo(m_ai);
    m_ai = nullptr;
}

bool CDNSResponse::empty() const
{
    return m_ai == nullptr;
}


CDNSResolve::~CDNSResolve() = default;

CDNSResponse::CDNSResponse(CDNSResponse&& rhs) noexcept
{
    m_ai = rhs.m_ai;
    rhs.m_ai = nullptr;
}

CDNSResponse& CDNSResponse::operator=(CDNSResponse&& rhs) noexcept
{
    clear();
    m_ai = rhs.m_ai;
    rhs.m_ai = nullptr;
    return *this;
}

event_type<evdns_getaddrinfo_request> CDNSResolve::Resolve(const event_type<evdns_base>& dns_base, const char* host, const char* port, const evutil_addrinfo* hints)
{
    assert(host != nullptr);
    assert(port != nullptr);
    // evdns_getaddrinfo may return NULL on success.
    event_type<evdns_getaddrinfo_request> result;
    result.reset(evdns_getaddrinfo(dns_base, host, port, hints, dns_callback, this));
    return result;
}

void CDNSResolve::dns_callback(int result, evutil_addrinfo* ai, void* ctx)
{
    if (result == EVUTIL_EAI_CANCEL) {
        // Do nothing. Class may no longer exist.
    } else if (result == DNS_ERR_NONE) {
        assert(ctx != nullptr);
        CDNSResolve* resolver = static_cast<CDNSResolve*>(ctx);
        resolver->OnResolveSuccess(CDNSResponse(ai));
    } else {
        assert(ctx != nullptr);
        CDNSResolve* resolver = static_cast<CDNSResolve*>(ctx);
        resolver->OnResolveFailure(result);
    }
}
