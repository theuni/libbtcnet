// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_PROXY_H
#define BITCOIN_NET_PROXY_H

#include <string>

struct bufferevent;
struct connection_data;

struct ProxyData
{
    std::string username;
    std::string password;
    std::string dest;
    unsigned short port;
    bool auth;
};

void proxy_init(bufferevent *bev, const connection_data* data);
#endif
