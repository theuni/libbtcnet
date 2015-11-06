// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_MESSAGE_H
#define BITCOIN_NET_MESSAGE_H

#include <stdint.h>
#include <stddef.h>
struct evbuffer;
struct CNetworkConfig;

void message_size(evbuffer *bev, size_t& nTotal, size_t& nComplete, uint32_t& nMessageSize, uint32_t& nMessageRemaining);
uint32_t first_complete_message_size(const CNetworkConfig& config, evbuffer *input, bool& fComplete, bool& fBadMsgStart);

static const unsigned int header_total_size = 24;
static const unsigned int header_size_offset = 16;

#endif // BITCOIN_NET_MESSAGE_H
