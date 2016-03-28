// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_CONNECTION_H
#define LIBBTCNET_CONNECTION_H

#include "networkconfig.h"

#include <string>
#include <vector>

struct sockaddr;

class CConnectionBase
{
protected:
    CConnectionBase(const sockaddr* addrIn, int addrlen);
    CConnectionBase(std::string hostIn, unsigned short portIn);
    CConnectionBase();

public:
    bool IsDNS() const;
    bool GetSockAddr(sockaddr* paddr, int* addrlen) const;
    std::string GetHost() const;
    unsigned short GetPort() const;
    bool IsSet() const;
    std::string ToString() const;

private:
    std::string host;
    unsigned short port;
    bool isDns;
    bool isSet;
    std::vector<unsigned char> addr;
};

class CProxyAuth
{
public:
    CProxyAuth(std::string usernameIn, std::string passwordIn);
    CProxyAuth();
    bool IsSet() const;
    const std::string& GetUsername() const;
    const std::string& GetPassword() const;

private:
    std::string username;
    std::string password;
    bool isSet;
};

class CProxy : public CConnectionBase
{
public:
    enum Type {
        SOCKS5 = 1,
        SOCKS5_FORCE_DOMAIN_TYPE = 2
    };
    CProxy(sockaddr* addrIn, int socksize, Type proxytype, CProxyAuth authIn = CProxyAuth());
    CProxy(std::string hostIn, unsigned short portIn, Type proxytype, CProxyAuth authIn = CProxyAuth());
    CProxy();
    const CProxyAuth& GetAuth() const;
    Type GetType() const;

private:
    CProxyAuth auth;
    Type type;
};

class CRateLimit
{
public:
    CRateLimit();
    size_t nMaxBurstRead;
    size_t nMaxReadRate;
    size_t nMaxBurstWrite;
    size_t nMaxWriteRate;
};

class CConnectionOptions
{
public:
    enum Family {
        UNSPEC = 1 << 0,
        IPV4 = 1 << 1,
        IPV6 = 1 << 2,
        ONION = 1 << 3,
        LOCAL = 1 << 4
    };

    enum Resolve {
        NO_RESOLVE = 1 << 0,
        RESOLVE_ONLY = 1 << 1,
        RESOLVE_CONNECT = 1 << 2
    };

    CConnectionOptions();
    bool fWhitelisted;
    bool fOneShot;
    bool fPersistent;
    Resolve doResolve;
    int nRetries;
    int nConnTimeout;
    int nRecvTimeout;
    int nSendTimeout;
    int nInitialTimeout;
    int nMaxSendBuffer;
    int nRetryInterval;
    int nMaxLookupResults;
    Family resolveFamily;
};

class CConnection : public CConnectionBase
{
public:
    CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, const sockaddr* addr, int socksize);
    CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, CProxy proxyIn, const sockaddr* addr, int socksize);
    CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, std::string addr, unsigned short port);
    CConnection(CConnectionOptions optsIn, CNetworkConfig netConfigIn, CProxy proxyIn, std::string addr, unsigned short port);
    CConnection();
    const CProxy& GetProxy() const;
    const CConnectionOptions& GetOptions() const;
    const CNetworkConfig& GetNetConfig() const;

private:
    CProxy proxy;
    CConnectionOptions opts;
    CNetworkConfig netConfig;
};

#endif // LIBBTCNET_CONNECTION_H
