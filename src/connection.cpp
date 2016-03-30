// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libbtcnet/connection.h"

#include <event2/util.h>

#include <string.h>
#include <assert.h>
#include <array>
#include <cstdlib>
#include <string>


#if defined(_WIN32)
#include <ws2tcpip.h>
#endif


CProxyAuth::CProxyAuth(std::string usernameIn, std::string passwordIn)
    : username(std::move(usernameIn)), password(std::move(passwordIn)), isSet(true) {}

CProxyAuth::CProxyAuth()
    : isSet(false) {}

bool CProxyAuth::IsSet() const
{
    return isSet;
}

const std::string& CProxyAuth::GetUsername() const
{
    return username;
}

const std::string& CProxyAuth::GetPassword() const
{
    return password;
}

CProxy::CProxy(sockaddr* addrIn, int socksize, Type proxytype, CProxyAuth authIn)
    : CConnectionBase(addrIn, socksize), auth(std::move(authIn)), type(proxytype)
{
}

CProxy::CProxy(std::string hostIn, unsigned short portIn, Type proxytype, CProxyAuth authIn)
    : CConnectionBase(std::move(hostIn), portIn), auth(std::move(authIn)), type(proxytype)
{
}

CProxy::CProxy()
    : type(SOCKS5)
{
}

const CProxyAuth& CProxy::GetAuth() const
{
    return auth;
}

CProxy::Type CProxy::GetType() const
{
    return type;
}

CConnectionBase::CConnectionBase(const sockaddr* addrIn, int addrlen)
    : port(0), isDns(false), isSet(true), addr(reinterpret_cast<const unsigned char*>(addrIn), reinterpret_cast<const unsigned char*>(addrIn) + addrlen)
{
}

CConnectionBase::CConnectionBase(std::string hostIn, unsigned short portIn)
    : host(std::move(hostIn)), port(portIn), isDns(true), isSet(true)
{
    sockaddr_storage saddr;
    int addrsize = sizeof(saddr);

    if (host.empty()) {
        *this = CConnectionBase();
        return;
    }

    if (evutil_parse_sockaddr_port(host.c_str(), reinterpret_cast<sockaddr*>(&saddr), &addrsize) == 0) {
        unsigned short* pport = nullptr;
        if (saddr.ss_family == AF_INET) {
            pport = &(reinterpret_cast<sockaddr_in*>(&saddr)->sin_port);

        } else if (saddr.ss_family == AF_INET6) {
            pport = &(reinterpret_cast<sockaddr_in6*>(&saddr)->sin6_port);
        }
        if ((pport != nullptr) && *pport == 0)
            *pport = ntohs(portIn);
        host.clear();
        port = 0;
        isDns = false;
        addr.assign(reinterpret_cast<unsigned char*>(&saddr), reinterpret_cast<unsigned char*>(&saddr) + addrsize);
    } else {
        size_t colon = host.find_last_of(':');
        bool fHaveColon = colon != host.npos;
        bool fMultiColon = fHaveColon && (host.find_last_of(':', colon - 1) != host.npos);
        if (fHaveColon && (colon == 0 || !fMultiColon)) {
            std::string portstr = host.substr(colon + 1);
            if (!portstr.empty()) {
                port = 0;
                try {
                    unsigned long longport = std::stoul(portstr);
                    if (longport <= 65535)
                        port = longport;
                } catch (...) {
                }
            }
            host = host.substr(0, colon);
        }
    }
}

CConnectionBase::CConnectionBase()
    : port(0), isDns(false), isSet(false)
{
}

bool CConnectionBase::IsDNS() const
{
    return isDns;
}

bool CConnectionBase::GetSockAddr(sockaddr* paddr, int* addrlen) const
{
    if (!addr.empty() && (addrlen != nullptr) && static_cast<size_t>(*addrlen) >= addr.size()) {
        memcpy(paddr, &addr[0], addr.size());
        *addrlen = addr.size();
    }
    return false;
}

std::string CConnectionBase::GetHost() const
{
    if (isDns)
        return host;

    std::string rethost;
    if (!addr.empty()) {
        std::array<char, NI_MAXHOST> hostbuf{};
        sockaddr_storage addr_stor;
        memset(&addr_stor, 0, sizeof(addr_stor));
        memcpy(&addr_stor, addr.data(), addr.size());
        if (getnameinfo(reinterpret_cast<sockaddr*>(&addr_stor), addr.size(), hostbuf.data(), hostbuf.size(), nullptr, 0, NI_NUMERICHOST) == 0)
            rethost.assign(hostbuf.data());
    }
    return rethost;
}

unsigned short CConnectionBase::GetPort() const
{
    if (isDns)
        return port;

    if (!addr.empty()) {
        std::array<char, NI_MAXSERV> servbuf{};
        sockaddr_storage addr_stor;
        memset(&addr_stor, 0, sizeof(addr_stor));
        memcpy(&addr_stor, addr.data(), addr.size());
        if (getnameinfo(reinterpret_cast<sockaddr*>(&addr_stor), addr.size(), nullptr, 0, servbuf.data(), servbuf.size(), NI_NUMERICSERV) == 0)
            return std::atoi(servbuf.data());
    }
    return 0;
}

bool CConnectionBase::IsSet() const
{
    return isSet;
}

std::string CConnectionBase::ToString() const
{
    if (isDns)
        return host + ":" + std::to_string(port);
    std::string ret;
    if (!addr.empty()) {
        std::array<char, NI_MAXHOST> hostbuf{};
        std::array<char, NI_MAXSERV> servbuf{};
        sockaddr_storage addr_stor;
        memset(&addr_stor, 0, sizeof(addr_stor));
        memcpy(&addr_stor, addr.data(), addr.size());
        if (getnameinfo(reinterpret_cast<sockaddr*>(&addr_stor), addr.size(), hostbuf.data(), hostbuf.size(), servbuf.data(), servbuf.size(), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
            size_t len = strlen(hostbuf.data()) + strlen(servbuf.data()) + 1;
            ret.reserve(len);
            ret.assign(hostbuf.data());
            ret.push_back(':');
            ret.append(servbuf.data());
        }
    }
    return ret;
}

CConnectionOptions::CConnectionOptions()
    : fWhitelisted(false), fOneShot(false), fPersistent(false), doResolve(NO_RESOLVE), nRetries(0), nConnTimeout(5), nRecvTimeout(60 * 20), nSendTimeout(60 * 20), nInitialTimeout(60), nMaxSendBuffer(5000000), nRetryInterval(1), nMaxLookupResults(0), resolveFamily(UNSPEC)
{
}

CRateLimit::CRateLimit()
    : nMaxBurstRead(EV_SSIZE_MAX), nMaxReadRate(EV_SSIZE_MAX), nMaxBurstWrite(EV_SSIZE_MAX), nMaxWriteRate(EV_SSIZE_MAX)
{
}
CConnection::CConnection()
{
}

CConnection::CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, const sockaddr* addr, int socksize)
    : CConnectionBase(addr, socksize), opts(optsIn), netConfig(std::move(netConfigIn))
{
}

CConnection::CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, CProxy proxyIn, const sockaddr* addr, int socksize)
    : CConnectionBase(addr, socksize), proxy(std::move(proxyIn)), opts(optsIn), netConfig(std::move(netConfigIn))
{
}

CConnection::CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, std::string addr, unsigned short port)
    : CConnectionBase(std::move(addr), port), opts(optsIn), netConfig(std::move(netConfigIn))
{
}

CConnection::CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, CProxy proxyIn, std::string addr, unsigned short port)
    : CConnectionBase(std::move(addr), port), proxy(std::move(proxyIn)), opts(optsIn), netConfig(std::move(netConfigIn))
{
}

const CProxy& CConnection::GetProxy() const
{
    return proxy;
}

const CConnectionOptions& CConnection::GetOptions() const
{
    return opts;
}

const CNetworkConfig& CConnection::GetNetConfig() const
{
    return netConfig;
}
