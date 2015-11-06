// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRESS_H
#define BITCOIN_ADDRESS_H

#include "serialize.h"
#include "service.h"

/** A CService with information about it as peer */
class CAddress : public CService
{
public:
    ADD_SERIALIZE_METHODS;

    CAddress() : nServices(0), nTime(0){}
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        if (ser_action.ForRead())
            nTime = 100000000;
        if (nType & SER_DISK)
            READWRITE(nVersion);
        if ((nType & SER_DISK) ||
            (nVersion >= CADDR_TIME_VERSION && !(nType & SER_GETHASH)))
            READWRITE(nTime);
        READWRITE(nServices);
        READWRITE(*(CService*)this);
    }

    static const int CADDR_TIME_VERSION = 31402;
    uint64_t nServices;

    // disk and network only
    unsigned int nTime;
};

#endif
