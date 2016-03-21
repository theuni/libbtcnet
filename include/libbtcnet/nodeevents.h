// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_NODEEVENTS_H
#define BTCNET_NODEEVENTS_H

#include <list>
#include <stddef.h>
#include <vector>

class CNodeEvents
{
public:
    virtual ~CNodeEvents();
    virtual void OnReadyForFirstSend() = 0;
    virtual bool OnReceiveMessages(std::list<std::vector<unsigned char> > msgs, size_t totalsize) = 0;
    virtual void OnWriteBufferFull(size_t bufsize) = 0;
    virtual void OnWriteBufferReady(size_t bufsize) = 0;
    virtual void OnBytesRead(size_t bytes, size_t total_bytes) = 0;
    virtual void OnBytesWritten(size_t bytes, size_t total_bytes) = 0;
    virtual void OnPingTimeout() = 0;
private:
    
};

#endif // BTCNET_NODEEVENTS_H
