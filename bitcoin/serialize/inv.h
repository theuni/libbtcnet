// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MESSAGES_INV_H
#define BITCOIN_MESSAGES_INV_H

#include "serialize.h"
#include "types/inv.h"

class CMsgInv
{
public:

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nInv)
    {
        READWRITE(vInv);
    }

    std::vector<CInv> vInv;
};

#endif // BITCOIN_INV_H
