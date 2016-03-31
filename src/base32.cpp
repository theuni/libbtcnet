// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base32.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <errno.h>

std::string EncodeBase32(const unsigned char* pch, size_t len)
{
    static const char* pbase32 = "abcdefghijklmnopqrstuvwxyz234567";

    std::string strRet = "";
    strRet.reserve((len + 4) / 5 * 8);

    int mode = 0, left = 0;
    const unsigned char* pchEnd = pch + len;

    while (pch < pchEnd) {
        int enc = *(pch++);
        switch (mode) {
        case 0: // we have no bits
            strRet += pbase32[enc >> 3];
            left = (enc & 7) << 2;
            mode = 1;
            break;

        case 1: // we have three bits
            strRet += pbase32[left | (enc >> 6)];
            strRet += pbase32[(enc >> 1) & 31];
            left = (enc & 1) << 4;
            mode = 2;
            break;

        case 2: // we have one bit
            strRet += pbase32[left | (enc >> 4)];
            left = (enc & 15) << 1;
            mode = 3;
            break;

        case 3: // we have four bits
            strRet += pbase32[left | (enc >> 7)];
            strRet += pbase32[(enc >> 2) & 31];
            left = (enc & 3) << 3;
            mode = 4;
            break;

        case 4: // we have two bits
            strRet += pbase32[left | (enc >> 5)];
            strRet += pbase32[enc & 31];
            mode = 0;
        }
    }

    static constexpr std::array<int, 5> nPadding{{0, 6, 4, 3, 1}};
    if (mode != 0) {
        strRet += pbase32[left];
        for (int n = 0; n < nPadding.at(mode); n++)
            strRet += '=';
    }

    return strRet;
}

std::string EncodeBase32(const std::string& str)
{
    return EncodeBase32(reinterpret_cast<const unsigned char*>(str.c_str()), str.size());
}

std::vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid)
{
    static constexpr std::array<int, 256> decode32_table{
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

    if (pfInvalid != nullptr)
        *pfInvalid = false;

    std::vector<unsigned char> vchRet;
    vchRet.reserve((strlen(p)) * 5 / 8);

    int mode = 0;
    int left = 0;

    while (true) {
        int dec = decode32_table.at(static_cast<unsigned char>(p[0]));
        if (dec == -1)
            break;
        p++;
        switch (mode) {
        case 0: // we have no bits and get 5
            left = dec;
            mode = 1;
            break;

        case 1: // we have 5 bits and keep 2
            vchRet.push_back((left << 3) | (dec >> 2));
            left = dec & 3;
            mode = 2;
            break;

        case 2: // we have 2 bits and keep 7
            left = left << 5 | dec;
            mode = 3;
            break;

        case 3: // we have 7 bits and keep 4
            vchRet.push_back((left << 1) | (dec >> 4));
            left = dec & 15;
            mode = 4;
            break;

        case 4: // we have 4 bits, and keep 1
            vchRet.push_back((left << 4) | (dec >> 1));
            left = dec & 1;
            mode = 5;
            break;

        case 5: // we have 1 bit, and keep 6
            left = left << 5 | dec;
            mode = 6;
            break;

        case 6: // we have 6 bits, and keep 3
            vchRet.push_back((left << 2) | (dec >> 3));
            left = dec & 7;
            mode = 7;
            break;

        case 7: // we have 3 bits, and keep 0
            vchRet.push_back((left << 5) | dec);
            mode = 0;
            break;
        }
    }

    if (pfInvalid != nullptr)
        switch (mode) {
        case 0: // 8n base32 characters processed: ok
            break;

        case 1: // 8n+1 base32 characters processed: impossible
        case 3: //   +3
        case 6: //   +6
            *pfInvalid = true;
            break;

        case 2: // 8n+2 base32 characters processed: require '======'
            if (left != 0 || p[0] != '=' || p[1] != '=' || p[2] != '=' || p[3] != '=' || p[4] != '=' || p[5] != '=' || decode32_table.at(static_cast<unsigned char>(p[6])) != -1)
                *pfInvalid = true;
            break;

        case 4: // 8n+4 base32 characters processed: require '===='
            if (left != 0 || p[0] != '=' || p[1] != '=' || p[2] != '=' || p[3] != '=' || decode32_table.at(static_cast<unsigned char>(p[4])) != -1)
                *pfInvalid = true;
            break;

        case 5: // 8n+5 base32 characters processed: require '==='
            if (left != 0 || p[0] != '=' || p[1] != '=' || p[2] != '=' || decode32_table.at(static_cast<unsigned char>(p[3])) != -1)
                *pfInvalid = true;
            break;

        case 7: // 8n+7 base32 characters processed: require '='
            if (left != 0 || p[0] != '=' || decode32_table.at(static_cast<unsigned char>(p[1])) != -1)
                *pfInvalid = true;
            break;
        }

    return vchRet;
}

std::string DecodeBase32(const std::string& str)
{
    std::vector<unsigned char> vchRet = DecodeBase32(str.c_str());
    return (vchRet.empty()) ? std::string() : std::string(reinterpret_cast<char*>(vchRet.data()), vchRet.size());
}
