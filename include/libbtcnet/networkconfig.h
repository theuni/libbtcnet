// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_NETWORKCONFIG_H
#define LIBBTCNET_NETWORKCONFIG_H

#include <vector>

struct CNetworkConfig {
    int header_msg_string_offset;
    int header_msg_size_offset;
    int header_msg_size_size;
    int header_msg_string_size;
    int header_size;
    unsigned int message_max_size;
    std::vector<unsigned char> message_start;

    int protocol_version;
    int protocol_handshake_version;
    int service_flags;
};

#endif // LIBBTCNET_NETWORKCONFIG_H
