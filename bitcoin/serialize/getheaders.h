// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MESSAGES_GETHEADERS_H
#define BITCOIN_MESSAGES_GETHEADERS_H

#include "serialize.h"
#include "blocklocator.h"
#include "bindata.h"

struct CGetHeaders
{
    /** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
     *  less than this number, we reached its tip. Changing this value is a protocol upgrade. */
    static const unsigned int MAX_HEADERS_RESULTS = 2000;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(locator);
        READWRITE(hashStop);
    }
    
    CBlockLocator locator;
    CBinData<256> hashStop;
};

#endif // BITCOIN_MESSAGES_GETHEADERS_H
