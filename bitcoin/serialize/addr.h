// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MESSAGES_ADDR_H
#define BITCOIN_MESSAGES_ADDR_H

#include "serialize.h"
#include "types/address.h"

class CMsgAddr
{
public:

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nAddr)
    {
        READWRITE(vAddress);
    }

    std::vector<CAddress> vAddress;
};

#endif // BITCOIN_ADDR_H
