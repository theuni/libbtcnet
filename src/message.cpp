// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "message.h"
#include "libbtcnet/connection.h"

#include <event2/buffer.h>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

namespace
{
static inline uint64_t get_message_length(const unsigned char* buf)
{
    return static_cast<const uint64_t>(buf[0]) |
           static_cast<const uint64_t>(buf[1]) << 8 |
           static_cast<const uint64_t>(buf[2]) << 16 |
           static_cast<const uint64_t>(buf[3]) << 24;
}
} // namespace

uint64_t first_complete_message_size(const CNetworkConfig& config, evbuffer* input, bool& fComplete, bool& fBadMsgStart)
{
    size_t nTotal = evbuffer_get_length(input);
    uint64_t nMessageSize;
    const int size_needed = config.header_msg_size_offset + config.header_msg_size_size;
    fComplete = false;
    fBadMsgStart = false;

    // Assume 4-bytes until there's a reason not to.
    assert(config.header_msg_size_size == 4);

    if (nTotal < static_cast<size_t>(size_needed))
        return 0;
    evbuffer_iovec v;
    if (evbuffer_peek(input, size_needed, nullptr, &v, 1) == 1) {
        const unsigned char* ptr = static_cast<const unsigned char*>(v.iov_base);
        if (!config.message_start.empty() && memcmp(ptr, &config.message_start[0], config.message_start.size()) != 0) {
            fBadMsgStart = true;
            return 0;
        }
        nMessageSize = get_message_length(ptr + config.header_msg_size_offset) + config.header_size;
    } else {
        std::vector<unsigned char> partial_header(size_needed);
        int ret = evbuffer_copyout(input, &partial_header[0], size_needed);
        assert(ret == size_needed);
        (void)ret;
        if (!config.message_start.empty() && memcmp(&partial_header[0], &config.message_start[0], config.message_start.size()) != 0) {
            fBadMsgStart = true;
            return 0;
        }
        nMessageSize = get_message_length(&partial_header[0] + config.header_msg_size_offset) + config.header_size;
    }
    if (nTotal >= nMessageSize)
        fComplete = true;
    return nMessageSize;
}
