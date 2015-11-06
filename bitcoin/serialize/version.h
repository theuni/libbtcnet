// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#include "serialize.h"
#include "address.h"

class CMessageVersion
{
public:
    CMessageVersion() 
    : nTime(0)
    , nNonce(0)
    , nVersion(0)
    , nServices(0)
    , fRelayTxes(0)
    , nStartingHeight(0)
    {}

    static const unsigned int MAX_SUBVERSION_LENGTH = 256;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(this->nVersion);
        if (ser_action.ForRead() && this->nVersion == 10300)
            this->nVersion = 300;

        READWRITE(nServices);
        READWRITE(nTime);

        if(nVersion < 106)
            return;

        if (ser_action.ForRead())
        {
            if(s.size())
                READWRITE(addrMe);

            if(s.size())
                READWRITE(addrFrom);

            if(s.size())
                READWRITE(nNonce);

            if(s.size())
                READWRITE(LIMITED_STRING(strSubVer, MAX_SUBVERSION_LENGTH));
            if(s.size())
                READWRITE(nStartingHeight);

            if(this->nVersion >= 70001 && s.size())
                READWRITE(fRelayTxes);
        }
        else
        {
            READWRITE(addrMe);
            READWRITE(addrFrom);
            READWRITE(nNonce);
            READWRITE(LIMITED_STRING(strSubVer, MAX_SUBVERSION_LENGTH));
            READWRITE(nStartingHeight);
            if(this->nVersion >= 70001)
                READWRITE(fRelayTxes);
        }
    }

    int64_t nTime;
    uint64_t nNonce;
    int nVersion;
    uint64_t nServices;
    bool fRelayTxes;
    int nStartingHeight;
    std::string strSubVer;
    CAddress addrMe;
    CAddress addrFrom;
};

#endif
