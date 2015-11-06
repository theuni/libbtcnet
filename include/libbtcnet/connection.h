// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_CONNECTION_H
#define BITCOIN_NET_CONNECTION_H

#include "networkconfig.h"

#include <string>
#include <vector>

struct sockaddr;

class CConnectionBase
{
protected:
    CConnectionBase(const sockaddr *addr, int socksize_in);
    CConnectionBase(const std::string& addr, unsigned short port);
    CConnectionBase();
public:

    bool IsDNS() const;
    bool GetSockAddr(sockaddr* paddr, int *addrlen) const;
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
    CProxyAuth(const std::string& username, const std::string& password);
    CProxyAuth();
    bool IsSet() const;
    std::string GetUsername() const;
    std::string GetPassword() const;
private:
    std::string username;
    std::string password;
    bool isSet;
};

class CProxy : public CConnectionBase
{
public:
    enum Type
    {
        SOCKS5 = 1 << 0,
        NODE =   1 << 1
    };
    CProxy(sockaddr* proxyaddr, int proxysocksize, Type proxytype, CProxyAuth auth = CProxyAuth());
    CProxy(const std::string& proxyhost, unsigned short proxyport, Type proxytype, CProxyAuth auth = CProxyAuth());
    CProxy();
    CProxyAuth GetAuth() const;
    Type GetType() const;
private:
    CProxyAuth auth;
    Type type;
};

class CConnectionOptions
{
    static const long nDefaultTimeout = 3;
public:

    enum Family
    {
        UNSPEC = 1 << 0,
        IPV4 =   1 << 1,
        IPV6 =   1 << 2
    };

    CConnectionOptions();
    int nRetries;
    bool fWhitelisted;
    bool fPersistent;
    bool fLookupOnly;
    int nConnTimeout;
    int nRecvTimeout;
    int nSendTimeout;
    int nInitialTimeout;
    int nMaxSendBuffer;
    int nMaxConnections;
    int nRetryInterval;
    int nMaxLookupResults;
    Family resolveFamily;
};

class CConnectionListener : public CConnectionBase
{
public:
    CConnectionListener();
    CConnectionListener(const sockaddr *addr, int socksize, const CConnectionOptions& optsIn, const CNetworkConfig& netconfigIn);
    CConnectionOptions GetOptions() const;
    CNetworkConfig GetNetConfig() const;
private:
    CConnectionOptions opts;
    CNetworkConfig netConfig;
};

class CConnection : public CConnectionBase
{
public:
    CConnection();
    CConnection(const sockaddr *addr, int socksize, const CConnectionOptions& opts, const CNetworkConfig& netconfig);
    CConnection(const sockaddr *addr, int socksize, const CConnectionOptions& opts, const CNetworkConfig& netconfig, const CProxy& proxy);
    CConnection(const std::string& addr, unsigned short port, const CConnectionOptions& opts, const CNetworkConfig& netconfig);
    CConnection(const std::string& addr, unsigned short port, const CConnectionOptions& opts, const CNetworkConfig& netconfig, const CProxy& proxy);

    CProxy GetProxy() const;
    CConnectionOptions GetOptions() const;
    CNetworkConfig GetNetConfig() const;
private:
    CProxy proxy;
    CConnectionOptions opts;
    CNetworkConfig netConfig;
};

#endif // BITCOIN_NET_CONNECTION_H
