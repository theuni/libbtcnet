#include "libbtcnet/networkconfig.h"
void WriteHeader(unsigned char* hdr, const CNetworkConfig& config, uint32_t size, const std::string& command)
{
    if(!config.message_start.empty())
        memcpy(hdr, &config.message_start[0], config.message_start.size());
    memcpy(hdr + config.header_msg_string_offset, command.c_str(), std::min((int)command.size(), config.header_msg_string_size)); 

    unsigned char* hdrsize = hdr + config.header_msg_size_offset;
    hdrsize[0] = size >>  0;
    hdrsize[1] = size >>  8;
    hdrsize[2] = size >> 16;
    hdrsize[3] = size >> 24;
}
