// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Utilities for converting data from/to strings.
 */
#ifndef BITCOIN_BASE32_H
#define BITCOIN_BASE32_H

#include <string>
#include <vector>

std::string EncodeBase32(const std::string& str);
std::string DecodeBase32(const std::string& str);
std::vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid = nullptr);
std::string EncodeBase32(const unsigned char* pch, size_t len);

#endif // BITCOIN_BASE32_H
