#include "helpers/simpleconnman.h"

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

/*
Sample Bootstrap demo.

Add 3 DNS Seed entries. Tell the connection manager to establish up to 10
simultaneous outgoing connections. The dummy addrman attempts to feed the
connection manager new addresses until it runs out, at which point it will
resolve more ips from the next seed node. Realistically, only 1 node should
be used.

After establishing 5 connections, shut down.

With added serialization/deserialization, this would be enough to create a
simplistic crawler.
*/

class CConnMan : public CSimpleConnManager
{
public:
    CConnMan(int max_outgoing) : CSimpleConnManager(max_outgoing){}
private:
    void ProcessMessage(CSimpleNode* node, const std::vector<unsigned char>& inmsg);
    void OnReadyForFirstSend(const CConnection& conn, uint64_t id);
    bool OnNeedOutgoingConnection(CConnection& conn, uint64_t id);
    void OnDnsResponse(const CConnection& conn, uint64_t id, const std::deque<CConnection>& results);
};

void CConnMan::OnDnsResponse(const CConnection& conn, uint64_t, const std::deque<CConnection>& results)
{
    /* An async dns response has arrived. Add the results to the dummy addrman */

    printf("Resolved %lu ips from %s.\n", results.size(), conn.GetHost().c_str());
    for(std::deque<CConnection>::const_iterator it = results.begin(); it != results.end(); ++it)
        AddAddress(*it);
}

bool CConnMan::OnNeedOutgoingConnection(CConnection& conn, uint64_t)
{
    /* Grab the next IP from the dummy addrman */
    conn = GetAddress();
    return conn.IsSet();
}

void CConnMan::OnReadyForFirstSend(const CConnection& conn, uint64_t id)
{
    printf("ready to send to node %lu. Address: %s:%i\n", id, conn.GetHost().c_str(), conn.GetPort());

    /* Outgoing version message goes here. Instead, just disconnect. */
    CloseConnection(id, false);

    static int messages = 0;
    if(messages++ == 5)
        Stop();
}

void CConnMan::ProcessMessage(CSimpleNode*, const std::vector<unsigned char>&)
{
    /* Messages come in here on the same thread. */
}

int main()
{
    /* Establish the connection options. See libbtcnet/connection.h for more. */
    CConnectionOptions myopts;
    myopts.nRetries = 5;
    myopts.fLookupOnly = true;

    /* Establish the network options. Notice that these are per-connection,
       though they're inherited as necessary. So a DNS Query on a main-net seed
       will return connections with main-net options. A connection manager will
       happily connect to multiple different networks simultaneously. The only
       requirement is that the header structure is similar enough to Bitcoin's
       that it can be expressed in the parameters below.
 */

    CNetworkConfig mainNet;
    mainNet.header_msg_string_offset=4;
    mainNet.header_msg_size_offset=16;
    mainNet.header_msg_size_size=4;
    mainNet.header_msg_string_size=12;
    mainNet.header_size=24;
    mainNet.message_max_size=1000000 + mainNet.header_size;
    mainNet.message_start.reserve(4);
    mainNet.message_start.push_back(0xf9);
    mainNet.message_start.push_back(0xbe);
    mainNet.message_start.push_back(0xb4);
    mainNet.message_start.push_back(0xd9);

    /* Connect to up to 10 outgoing nodes. For this demo, listening is disabled */
    CConnMan connman(10);

    connman.AddSeed(CConnection("seed.bitcoin.sipa.be", 8333, myopts, mainNet));
    connman.AddSeed(CConnection("dnsseed.bluematt.me", 8333, myopts, mainNet));
    connman.AddSeed(CConnection("dnsseed.bitcoin.dashjr.org", 8333, myopts, mainNet));

    connman.Start();
}
