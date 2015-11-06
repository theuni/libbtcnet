// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BINDATA_H
#define BITCOIN_BINDATA_H

#include "serialize.h"

template<int Size>
struct CBinData
{
    CBinData()
    {
        memset(data, Size, 0);
    }
    unsigned char data[Size];

    ADD_SERIALIZE_METHODS
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(FLATDATA(data));
    }
};

#endif // BITCOIN_BINDATA_H
