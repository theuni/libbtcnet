// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKHEADER_H
#define BITCOIN_BLOCKHEADER_H

#include "serialize.h"
#include "bindata.h"

class CBlockHeader
{
public:
    static const int32_t CURRENT_VERSION=3;
    int32_t nVersion;
    CBinData<256> hashPrevBlock;
    CBinData<256> hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CBlockHeader() : nVersion(0), nTime(0), nBits(0), nNonce(0){}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }
};


#endif // BITCOIN_BLOCKHEADER_H
