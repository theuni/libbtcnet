// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libbtcnet/connection.h"

#include <event2/util.h>

#include <string.h>
#include <string>
#include <assert.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <sys/un.h>
#endif

CProxyAuth::CProxyAuth(const std::string& usernameIn, const std::string& passwordIn)
: username(usernameIn), password(passwordIn), isSet(true){}

CProxyAuth::CProxyAuth()
: username(""), password(""), isSet(false){}

bool CProxyAuth::IsSet() const
{
    return isSet;
}

std::string CProxyAuth::GetUsername() const
{
    return username;
}

std::string CProxyAuth::GetPassword() const
{
    return password;
}

CProxy::CProxy(sockaddr* addrIn, int socksize, Type proxytype, CProxyAuth authIn)
: CConnectionBase(addrIn, socksize), auth(authIn), type(proxytype)
{
}

CProxy::CProxy(const std::string& hostIn, unsigned short portIn, Type proxytype, CProxyAuth authIn)
: CConnectionBase(hostIn, portIn), auth(authIn), type(proxytype)
{
}

CProxy::CProxy()
: CConnectionBase(), auth(CProxyAuth()), type(SOCKS5)
{
}

CProxyAuth CProxy::GetAuth() const
{
    return auth;
}

CProxy::Type CProxy::GetType() const
{
    return type;
}

CConnectionBase::CConnectionBase(const sockaddr *addrIn, int addrlen)
: host(""), port(0), isDns(false), isSet(true)
, addr(reinterpret_cast<const unsigned char*>(addrIn), reinterpret_cast<const unsigned char*>(addrIn) + addrlen)
{
}

CConnectionBase::CConnectionBase(const std::string& hostIn, unsigned short portIn)
: host(hostIn), port(portIn), isDns(true), isSet(true)
{
    sockaddr_storage saddr;
    int addrsize = sizeof(saddr);

    if(hostIn.empty())
    {
        *this = CConnectionBase();
        return;
    }

    if(evutil_parse_sockaddr_port(hostIn.c_str(), (sockaddr*)&saddr, &addrsize) == 0)
    {
        unsigned short* pport = NULL;
        if(saddr.ss_family == AF_INET)
        {
            pport = &((sockaddr_in*)&saddr)->sin_port;
        }
        else if (saddr.ss_family == AF_INET6)
        {
            pport = &((sockaddr_in6*)&saddr)->sin6_port;
        }
        if(pport && *pport == 0)
            *pport = ntohs(portIn);
        host.clear();
        port = 0;
        isDns = false;
        addr.assign((unsigned char*)&saddr, (unsigned char*)&saddr + addrsize);
    }
    else
    {
        size_t colon = host.find_last_of(':');
        bool fHaveColon = colon != host.npos;
        bool fMultiColon = fHaveColon && (host.find_last_of(':',colon-1) != host.npos);
        if (fHaveColon && (colon==0 || !fMultiColon))
        {
            std::string portstr = host.substr(colon + 1);
            if(!portstr.empty())
            {
                // hack to avoid strtol
                evutil_addrinfo hint;
                memset(&hint, 0, sizeof(hint));
                hint.ai_socktype = SOCK_STREAM;
                hint.ai_protocol = IPPROTO_TCP;
                hint.ai_flags = EVUTIL_AI_NUMERICSERV;
                hint.ai_family = AF_INET;
                evutil_addrinfo* res = NULL;
                if(evutil_getaddrinfo(NULL, host.substr(colon + 1).c_str(), &hint, &res) == 0 && res && res->ai_addr)
                {
                    port = ntohs(((sockaddr_in*)res->ai_addr)->sin_port);
                    evutil_freeaddrinfo(res);
                }
                else
                    port = 0;
                host = host.substr(0, colon);
            }
        }
    }
}

CConnectionBase::CConnectionBase()
: host(""), port(0), isDns(false), isSet(false)
{
}

CConnectionBase::CConnectionBase(const CConnectionBase& rhs)
: port(rhs.port), isDns(rhs.isDns), isSet(rhs.isSet)
{
    if(isSet)
    {
        if(isDns)
            host.assign(rhs.host);
        else
            addr.assign(rhs.addr.begin(), rhs.addr.end()); 
    }
    
}

CConnectionBase& CConnectionBase::operator=(const CConnectionBase rhs)
{
    if(rhs.isSet)
    {
        isSet = true;
        if(rhs.isDns)
        {
            if(!isDns)
            {
                addr.clear();
                isDns = true;
            }
            host.assign(rhs.host);
            port = rhs.port;
        }
        else
        {
            if(isDns)
            {
                host.clear();
                port = 0;
                isDns = false;
            }
            addr.assign(rhs.addr.begin(), rhs.addr.end());
        }
    }
    else
    {
        addr.clear();
        host.clear();
        port = 0;
    }
    isSet = rhs.isSet;
    return *this;
}

bool CConnectionBase::IsDNS() const
{
    return isDns;
}

bool CConnectionBase::GetSockAddr(sockaddr* paddr, int *addrlen) const
{
    if(addr.size() && addrlen && (size_t)*addrlen >= addr.size())
    {
        memcpy(paddr, &addr[0], addr.size());
        *addrlen = addr.size();
    }
    return false;
}

std::string CConnectionBase::GetHost() const
{
    if(isDns)
        return host;
    else if (!addr.empty())
    {
        char hostbuf[NI_MAXHOST] = {};
        if (getnameinfo((sockaddr*)&addr[0], addr.size(), hostbuf, sizeof(hostbuf), NULL, 0, NI_NUMERICHOST) == 0)
            return std::string(hostbuf);
    }
    return std::string();
}

unsigned short CConnectionBase::GetPort() const
{
    if(isDns)
        return port;
    else if(addr.empty())
        return 0;
    else
    {
        unsigned short ret = 0;
        sockaddr_storage storage;
        memcpy(&storage, &addr[0], addr.size());
        sockaddr* saddr = (sockaddr*)&storage;
        if(saddr->sa_family == AF_INET)
            ret = ((sockaddr_in*)&storage)->sin_port;
        else if (saddr->sa_family == AF_INET6)
            ret = ((sockaddr_in6*)&storage)->sin6_port;
        return ntohs(ret);
    }
}

bool CConnectionBase::IsSet() const
{
    return isSet;
}

std::string CConnectionBase::ToString() const
{
    return GetHost();
}

CConnectionOptions::CConnectionOptions()
: fWhitelisted(false)
, fOneShot(false)
, fPersistent(false)
, doResolve(NO_RESOLVE)
, nRetries(0)
, nConnTimeout(5)
, nRecvTimeout(60 * 20)
, nSendTimeout(60 * 20)
, nInitialTimeout(60)
, nMaxSendBuffer(5000000)
, nRetryInterval(1)
, nMaxLookupResults(0)
, resolveFamily(UNSPEC)
{
}

CRateLimit::CRateLimit()
: nMaxBurstRead(EV_SSIZE_MAX)
, nMaxReadRate(EV_SSIZE_MAX)
, nMaxBurstWrite(EV_SSIZE_MAX)
, nMaxWriteRate(EV_SSIZE_MAX)
{
}

CConnectionListener::CConnectionListener()
{
}

CConnectionListener::CConnectionListener(const sockaddr *addrIn, int socksize, const CConnectionOptions& optsIn, const CNetworkConfig& netConfigIn)
: CConnectionBase(addrIn, socksize), opts(optsIn), netConfig(netConfigIn)
{
}

const CConnectionOptions& CConnectionListener::GetOptions() const
{
    return opts;
}

const CNetworkConfig& CConnectionListener::GetNetConfig() const
{
    return netConfig;
}

CConnection::CConnection(const sockaddr *addrIn, int socksize, const CConnectionOptions& optsIn, const CNetworkConfig& netConfigIn, const CProxy& proxyIn)
: CConnectionBase(addrIn, socksize)
, proxy(proxyIn), opts(optsIn), netConfig(netConfigIn)
{
}

CConnection::CConnection(const sockaddr *addrIn, int socksize, const CConnectionOptions& optsIn, const CNetworkConfig& netConfigIn)
: CConnectionBase(addrIn, socksize), opts(optsIn), netConfig(netConfigIn)
{
}

CConnection::CConnection(const std::string& hostIn, unsigned short portIn, const CConnectionOptions& optsIn, const CNetworkConfig& netConfigIn)
: CConnectionBase(hostIn, portIn), opts(optsIn), netConfig(netConfigIn)
{
}

CConnection::CConnection(const std::string& hostIn, unsigned short portIn, const CConnectionOptions& optsIn, const CNetworkConfig& netConfigIn, const CProxy& proxyIn)
: CConnectionBase(hostIn, portIn), proxy(proxyIn),  opts(optsIn), netConfig(netConfigIn)
{
}

CConnection::CConnection()
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
