// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_MESSAGE_H
#define BTCNET_MESSAGE_H

#include <stdint.h>

struct evbuffer;
struct CNetworkConfig;

uint64_t first_complete_message_size(const CNetworkConfig& config, evbuffer* input, bool& fComplete, bool& fBadMsgStart);

#endif // BTCNET_MESSAGE_H
