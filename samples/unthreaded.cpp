#include "helpers/simpleconnman.h"
#include "helpers/simplenode.h"
#include "bitcoin/serialize/version.h"
#include "bitcoin/serialize/address.h"
#include "bitcoin/serialize/netstream.h"
#include "bitcoin/serialize/messageheader.h"
#include "helpers/messaging.h"

#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#endif

/*
Sample Crawler demo.

Add 3 DNS Seed entries. Tell the connection manager to establish up to 10
simultaneous outgoing connections. The dummy addrman attempts to feed the
connection manager new addresses until it runs out, at which point it will
resolve more ips from the next seed node. Realistically, only 1 node should
be used.

After receiving some addresses from a peer, shut down.
*/

class CConnMan : public CSimpleConnManager
{
public:
    CConnMan() : CSimpleConnManager(5000000){}
private:
    void ProcessMessage(CSimpleNode* node, const std::vector<unsigned char>& inmsg);
    void ProcessFirstMessage(CSimpleNode* node, const std::vector<unsigned char>& inmsg);
    void OnReadyForFirstSend(CSimpleNode* node);
    bool OnNeedOutgoingConnections(std::vector<CConnection>& conns, int need_count);
    void OnDnsResponse(const CConnection& conn, uint64_t id, std::list<CConnection>& results);

    template <typename T>
    void WriteCommand(CSimpleNode* node, const std::string& command, const T& obj, uint32_t checksum);
    void WriteCommand(CSimpleNode* node, const std::string& command);
};

void CConnMan::OnDnsResponse(const CConnection& conn, uint64_t, std::list<CConnection>& results)
{
    /* An async dns response has arrived. Add the results to the dummy addrman */

    printf("Resolved %lu ips from %s.\n", results.size(), conn.GetHost().c_str());
    for(std::list<CConnection>::const_iterator it = results.begin(); it != results.end(); ++it)
    {
        AddAddress(*it);
    }
}

bool CConnMan::OnNeedOutgoingConnections(std::vector<CConnection>& conns, int need_count)
{
    /* Grab the next IP from the dummy addrman */
    CConnection conn;
    while(GetAddress(conn) && need_count--)
    {
        conns.push_back(conn);
    }
    return !conns.empty();
}

template <typename T>
void CConnMan::WriteCommand(CSimpleNode* node, const std::string& command, const T& obj, uint32_t checksum)
{
    CNetStream buf(SER_NETWORK, node->GetSendVersion());
    const CNetworkConfig& netConfig = node->GetConnection().GetNetConfig();
    buf.resize(netConfig.header_size, 0);
    WriteHeader(&buf[0], netConfig, buf.GetSerializeSize(obj), command);
    uint32_t lechecksum = htole32(checksum);
    memcpy(&buf[20], (char*)&lechecksum, 4);
    buf << obj;
    SendMsg(node->GetId(), (unsigned char*)&buf[0], buf.size());
}

void CConnMan::WriteCommand(CSimpleNode* node, const std::string& command)
{
    CNetStream buf(SER_NETWORK, node->GetSendVersion());
    const CNetworkConfig& netConfig = node->GetConnection().GetNetConfig();
    buf.resize(netConfig.header_size, 0);
    WriteHeader(&buf[0], netConfig, 0, command);
    uint32_t lechecksum = htole32(0xe2e0f65d);
    memcpy(&buf[20], (char*)&lechecksum, 4);
    SendMsg(node->GetId(), (unsigned char*)&buf[0], buf.size());
}

void CConnMan::OnReadyForFirstSend(CSimpleNode* node)
{
    const CConnection& connection = node->GetConnection();
    CMessageVersion version;
    version.nVersion = connection.GetNetConfig().protocol_version;
    version.strSubVer = "test node";
#if 0
//TODO: Need hashing first.
    sockaddr_storage storage;
    int sock_size = sizeof(storage);
    connection.GetSockAddr((sockaddr*)&storage, &sock_size);
    CAddress addr;
    addr.nServices = connection.GetNetConfig().service_flags;
    addr.ip[10] = addr.ip[11] = 0xff;

    if(storage.ss_family == AF_INET)
    {
        sockaddr_in* addr4 = (sockaddr_in*)&storage;
        memcpy(addr.ip + 12, &addr4->sin_addr, 4);
        addr.port = ntohs(addr4->sin_port);
    }
    else if(storage.ss_family == AF_INET6)
    {
        sockaddr_in6* addr6 = (sockaddr_in6*)&storage;
        memcpy(addr.ip, &addr6->sin6_addr, 16);
        addr.port = ntohs(addr6->sin6_port);
    }
    version.nVersion = connection.GetNetConfig().protocol_version;
    version.strSubVer = "test node";
    version.addrMe = addr;
    WriteCommand(node, "version", version, 0xd6b0c1ff);
#endif
    WriteCommand(node, "version", version, 0xf6f1e14f);
    node->SetOurVersion(version);
}

void CConnMan::ProcessFirstMessage(CSimpleNode* node, const std::vector<unsigned char>& msg)
{
    CNetStream buf(msg.begin(), msg.end(), SER_NETWORK, node->GetRecvVersion());
    CMessageHeader hdr;
    buf >> hdr;
    std::string command = std::string(hdr.pchCommand);

    if(command == "version")
    {
        CMessageVersion version;
        buf >> version;
        node->SetTheirVersion(version);
        if(node->IsIncoming())
        {
            OnReadyForFirstSend(node);
        }
        WriteCommand(node, "verack");
        node->UpgradeSendVersion();

        WriteCommand(node, "getaddr");
    }
    else
    {
        CloseConnection(node->GetId(), true);
        return;
    }
}

void CConnMan::ProcessMessage(CSimpleNode* node, const std::vector<unsigned char>& msg)
{
    CNetStream buf(msg.begin(), msg.end(), SER_NETWORK, node->GetRecvVersion());
    CMessageHeader hdr;
    buf >> hdr;
    std::string command = std::string(hdr.pchCommand);
    if(command == "version")
    {
        CloseConnection(node->GetId(), true);
        return;
    }
    else if(command == "addr")
    {
        CloseConnection(node->GetId(), false);
        std::vector<CAddress> addresses;
        buf >> addresses;
        printf("got : %lu addresses.\n", addresses.size());
        Stop();
    }
    else if(command == "verack")
    {
        node->UpgradeRecvVersion();
    }
}

int main()
{
#if defined(_WIN32)
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
#endif

    /* Establish the connection options. See libbtcnet/connection.h for more. */
    CConnectionOptions myopts;
    myopts.nRetries = 5;
    myopts.doResolve = CConnectionOptions::RESOLVE_ONLY;

    /* Establish the network options. Notice that these are per-connection,
       though they're inherited as necessary. So a DNS Query on a main-net seed
       will return connections with main-net options. A connection manager will
       happily connect to multiple different networks simultaneously. The only
       requirement is that the header structure is similar enough to Bitcoin's
       that it can be expressed in the parameters below.
     */

    static const unsigned char mainnet_msgstart[] = {0xf9, 0xbe, 0xb4, 0xd9};
    static const unsigned char testnet_msgstart[] = {0x0b, 0x11, 0x09, 0x07};
    CNetworkConfig mainNet;
    mainNet.header_msg_string_offset=4;
    mainNet.header_msg_size_offset=16;
    mainNet.header_msg_size_size=4;
    mainNet.header_msg_string_size=12;
    mainNet.header_size=24;
    mainNet.message_max_size=1000000 + mainNet.header_size;
    mainNet.message_start = std::vector<unsigned char>(mainnet_msgstart, mainnet_msgstart + sizeof(mainnet_msgstart));
    mainNet.protocol_version = 70011;
    mainNet.protocol_handshake_version = 209;
    mainNet.service_flags = 0;

    CNetworkConfig testNet = mainNet;
    testNet.message_start = std::vector<unsigned char>(testnet_msgstart, testnet_msgstart + sizeof(testnet_msgstart));

    /* Connect to up to 10 outgoing nodes. For this demo, listening is disabled */
    CConnMan connman;

    CRateLimit limit;
    limit.nMaxReadRate = 10000;
    limit.nMaxWriteRate = 10000;
    limit.nMaxBurstRead = 10000;
    limit.nMaxBurstWrite = 10000;
    connman.SetOutgoingRateLimit(limit);

    connman.AddSeed(CConnection("dnsseed.bluematt.me", 8333, myopts, mainNet));
    connman.AddSeed(CConnection("seed.bitcoin.sipa.be", 8333, myopts, mainNet));
    connman.AddSeed(CConnection("dnsseed.bitcoin.dashjr.org", 8333, myopts, mainNet));

    connman.Start(8, 0);
#ifdef _WIN32
    WSACleanup();
#endif
}
