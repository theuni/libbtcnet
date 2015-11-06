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
}

CConnectionBase::CConnectionBase()
: host(""), port(0), isDns(false), isSet(false)
{
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
    else if(addr.empty())
        return "";
    else
    {
        sockaddr_storage storage;
        memcpy(&storage, &addr[0], addr.size());
        sockaddr* saddr = (sockaddr*)&storage;
#if !defined(_WIN32)
        if(saddr->sa_family == AF_UNIX)
            return std::string(((sockaddr_un*)&storage)->sun_path);
#endif
        char str[INET6_ADDRSTRLEN] = {};
        void* sin = NULL;
        if(saddr->sa_family == AF_INET)
            sin = &(((struct sockaddr_in *)&storage)->sin_addr);
        else if (saddr->sa_family == AF_INET6)
            sin = &(((struct sockaddr_in6 *)&storage)->sin6_addr);
        if(sin)
            evutil_inet_ntop(saddr->sa_family, sin, str, sizeof(str));
        return std::string(str);
    }
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
: nRetries(0)
, fWhitelisted(false)
, fPersistent(false)
, fLookupOnly(false)
, nConnTimeout(5)
, nRecvTimeout(60 * 20)
, nSendTimeout(60 * 20)
, nInitialTimeout(60)
, nMaxSendBuffer(5000000)
, nMaxConnections(1)
, nRetryInterval(1)
, nMaxLookupResults(0)
, resolveFamily(UNSPEC)
{
}

CConnectionListener::CConnectionListener()
{
}

CConnectionListener::CConnectionListener(const sockaddr *addrIn, int socksize, const CConnectionOptions& optsIn, const CNetworkConfig& netConfigIn)
: CConnectionBase(addrIn, socksize), opts(optsIn), netConfig(netConfigIn)
{
}

CConnectionOptions CConnectionListener::GetOptions() const
{
    return opts;
}

CNetworkConfig CConnectionListener::GetNetConfig() const
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

CProxy CConnection::GetProxy() const
{
    return proxy;
}

CConnectionOptions CConnection::GetOptions() const
{
    return opts;
}

CNetworkConfig CConnection::GetNetConfig() const
{
    return netConfig;
}
