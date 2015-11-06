// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_NETWORKCONFIG_H
#define BITCOIN_NET_NETWORKCONFIG_H

#include <vector>

struct CNetworkConfig
{
    int header_msg_string_offset;
    int header_msg_size_offset;
    int header_msg_size_size;
    int header_msg_string_size;
    int header_size;
    unsigned int message_max_size;
    std::vector<unsigned char> message_start;
};

#endif // BITCOIN_NET_NETWORKCONFIG_H
