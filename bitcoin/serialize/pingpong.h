// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PINGPONG_H
#define BITCOIN_PINGPONG_H

#include "serialize.h"

class CPing
{
public:
    //! BIP 0031, pong message, is enabled for all versions AFTER this one
    static const int BIP0031_VERSION = 60000;

    CPing() : nNonce(0){}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (nVersion > BIP0031_VERSION)
            READWRITE(nNonce);
    }

    uint64_t nNonce;
};

typedef CPing CPong;

#endif // BITCOIN_PINGPONG_H
