// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MESSAGEBLOOMFILTER_H
#define BITCOIN_MESSAGEBLOOMFILTER_H

#include "serialize.h"

#include <vector>

class CBloomFilter
{
public:
    CBloomFilter() : nHashFuncs(0), nTweak(0), nFlags(0){}

    //! 20,000 items with fp rate < 0.1% or 10,000 items and <0.0001%
    static const unsigned int MAX_BLOOM_FILTER_SIZE = 36000; // bytes
    static const unsigned int MAX_HASH_FUNCS = 50;

    /**
     * First two bits of nFlags control how much IsRelevantAndUpdate actually updates
     * The remaining bits are reserved
     */
    enum bloomflags
    {
        BLOOM_UPDATE_NONE = 0,
        BLOOM_UPDATE_ALL = 1,
        // Only adds outpoints to the filter if the output is a pay-to-pubkey/pay-to-multisig script
        BLOOM_UPDATE_P2PUBKEY_ONLY = 2,
        BLOOM_UPDATE_MASK = 3,
    };

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vData);
        READWRITE(nHashFuncs);
        READWRITE(nTweak);
        READWRITE(nFlags);
    }

    std::vector<unsigned char> vData;
    unsigned int nHashFuncs;
    unsigned int nTweak;
    unsigned char nFlags;
};

#endif // BITCOIN_MESSAGEBLOOMFILTER_H
