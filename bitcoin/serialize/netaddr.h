// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETADDR_H
#define BITCOIN_NETADDR_H

#include "serialize.h"

#include <string.h>

/** IP address (IPv6, or IPv4 using mapped IPv6 range (::FFFF:0:0/96)) */
class CNetAddr
{
public:
    CNetAddr()
    {
        memset(&ip, 0, sizeof(ip));
    }
    unsigned char ip[16]; // in network byte order

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(FLATDATA(ip));
    }
};

#endif // BITCOIN_NETADDR_H
