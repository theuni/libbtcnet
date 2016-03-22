#include "libbtcnet/handler.h"
#include "libbtcnet/connection.h"
#include <list>

static const std::vector<unsigned char> mainnet_message_start = {0xf9, 0xbe, 0xb4, 0xd9};
static const std::vector<unsigned char> testnet_message_start = {0x0b, 0x11, 0x09, 0x07};

class CConnman final : public CConnectionHandler
{
public:
    CConnman(bool enable_threads)
        : CConnectionHandler(enable_threads)
    {
    }
    void Run(int max_outgoing)
    {
        Start(max_outgoing);
        while (PumpEvents(true))
            ;
    }

protected:
    std::list<CConnection> OnNeedOutgoingConnections(int need_count) final
    {
        std::list<CConnection> ret;
        if (!m_connections.empty())
            ret.splice(ret.end(), m_connections);
        return ret;
    }
    bool OnOutgoingConnection(ConnID id, const CConnection& conn, const CConnection& resolved_conn) final
    {
        if (resolved_conn.GetNetConfig().message_start == mainnet_message_start)
            printf("mainnet connected first\n");
        else if (resolved_conn.GetNetConfig().message_start == testnet_message_start)
            printf("testnet connected first\n");
        Shutdown();
        return true;
    }
    void OnShutdown() final
    {
        printf("all done\n");
    }
    void OnStartup()
    {
        /* Establish the network options. Notice that these are per-connection,
           though they're inherited as necessary. So a DNS Query on a main-net seed
           will return connections with main-net options. A connection manager will
           happily connect to multiple different networks simultaneously. The only
           requirement is that the header structure is similar enough to Bitcoin's
           that it can be expressed in the parameters below.
         */
        CNetworkConfig mainnet_config;
        mainnet_config.header_msg_string_offset = 4;
        mainnet_config.header_msg_size_offset = 16;
        mainnet_config.header_msg_size_size = 4;
        mainnet_config.header_msg_string_size = 12;
        mainnet_config.header_size = 24;
        mainnet_config.message_max_size = 1000000 + mainnet_config.header_size;
        mainnet_config.message_start = mainnet_message_start;
        mainnet_config.protocol_version = 70011;
        mainnet_config.protocol_handshake_version = 209;
        mainnet_config.service_flags = 0;

        CNetworkConfig testnet_config = mainnet_config;
        testnet_config.message_start = testnet_message_start;


        /* Establish the connection options. See libbtcnet/connection.h for more. */
        CConnectionOptions myopts;
        myopts.nRetries = 0;
        myopts.doResolve = CConnectionOptions::RESOLVE_CONNECT;
        myopts.nConnTimeout = 1;

        m_connections.emplace_back(myopts, mainnet_config, "seed.bitcoin.sipa.be", 8333);
        m_connections.emplace_back(myopts, testnet_config, "testnet-seed.bitcoin.petertodd.org", 8333);

        m_connections.emplace_back(myopts, mainnet_config, "dnsseed.bluematt.me", 8333);
        m_connections.emplace_back(myopts, testnet_config, "testnet-seed.bluematt.me", 8333);

        m_connections.emplace_back(myopts, mainnet_config, "dnsseed.bitcoin.dashjr.org", 8333);
        m_connections.emplace_back(myopts, testnet_config, "testnet-seed.bitcoin.schildbach.de", 8333);
    }

    bool OnIncomingConnection(ConnID id, const CConnection& listenconn, const CConnection& resolved_conn) final { return false; }
    void OnDnsResponse(const CConnection& conn, std::list<CConnection> results) final {}
    void OnConnectionFailure(const CConnection& conn, const CConnection& resolved, bool retry) final {}
    void OnDisconnected(ConnID id, bool persistent) final {}
    void OnBindFailure(const CConnection& listener) final {}
    void OnDnsFailure(const CConnection& conn, bool retry) final {}
    void OnWriteBufferFull(ConnID id, size_t bufsize) final {}
    void OnWriteBufferReady(ConnID id, size_t bufsize) final {}
    bool OnReceiveMessages(ConnID id, std::list<std::vector<unsigned char> > msgs, size_t totalsize) final { return false; }
    void OnMalformedMessage(ConnID id) final {}
    void OnReadyForFirstSend(ConnID id) final {}
    void OnProxyFailure(const CConnection& conn, bool retry) final {}
    void OnBytesRead(ConnID id, size_t bytes, size_t total_bytes) final {}
    void OnBytesWritten(ConnID id, size_t bytes, size_t total_bytes) final {}
    void OnPingTimeout(ConnID id){}

private:
    std::list<CConnection> m_connections;
};

int main()
{
    CConnman test(false);
    test.Run(8);
}
