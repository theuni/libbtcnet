// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SERVICE_H
#define BITCOIN_SERVICE_H

#include "serialize.h"
#include "netaddr.h"

/** A combination of a network address (CNetAddr) and a (TCP) port */
class CService : public CNetAddr
{
public:
    CService() : port(0){}

    unsigned short port; // host order

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(FLATDATA(ip));
        const unsigned char newport[2] = {(unsigned char)(port >> 8), (unsigned char)(port & 0xff)};
        READWRITE(FLATDATA(newport));
        if (ser_action.ForRead())
            port = (unsigned char)(newport[0]) << 8 | (unsigned short)(newport[1]);
        }
};

#endif // BITCOIN_SERVICE_H
