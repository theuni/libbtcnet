#include "libbtcnet/handler.h"
#include "libbtcnet/connection.h"
#include "libbtcnet/nodeevents.h"
#include <map>
#include <list>
#include <memory>
#include <thread>
#include <condition_variable>
#include <assert.h>


class CConnman final : public CConnectionHandler
{

    class CNode final : public CNodeEvents
    {
    public:
        public:
        CNode(CConnman& connman, ConnID id, CConnection conn, bool incoming) : m_connman(connman), m_id(id), m_conn(std::move(conn)), m_done(false), m_incoming(incoming), m_ready_for_first_send(false)
        {
            m_thread = std::thread(&CNode::Run, this);
            m_thread.detach();
        }
        void Run()
        {
            bool done = false;
            if(!m_incoming)
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condvar.wait(lock, [this]{return m_done || m_ready_for_first_send; });
                    done = m_done;
                }
                if(!done)
                    SendFirst();
            }
            while(!done)
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condvar.wait(lock, [this]{return m_done || !m_msgs.empty() || !m_processing_msg.empty(); });
                    if(m_done)
                        break;
                    if(m_processing_msg.empty())
                    {
                        assert(!m_msgs.empty());
                        m_processing_msg = std::move(m_msgs.front());
                        m_msgs.pop_front();
                    }
                }
                Process();
            }
            m_connman.Done(m_id);
        }

        void SendFirst()
        {
        }

        void Process()
        {
            m_processing_msg.clear();
        }

        void Stop()
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_done = true;
                m_msgs.clear();
            }
            m_condvar.notify_one();
        }
    protected:
        void OnReadyForFirstSend() final {
            m_condvar.notify_one();
        }
        bool OnReceiveMessages(std::list<std::vector<unsigned char> > msgs, size_t totalsize) final
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_msgs.splice(m_msgs.end(), msgs);
            }
            m_condvar.notify_one();
            return true;
        }
        void OnWriteBufferFull(size_t bufsize) final {}
        void OnWriteBufferReady(size_t bufsize) final {}
        void OnBytesRead(size_t bytes, size_t total_bytes) final {}
        void OnBytesWritten(size_t bytes, size_t total_bytes) final {}
        void OnPingTimeout(){}
    private:
        inline void Send(const unsigned char* data, size_t size) const { m_connman.Send(m_id, data, size); }
        CConnman& m_connman;
        const ConnID m_id;
        CConnection m_conn;
        std::condition_variable m_condvar;
        std::mutex m_mutex;
        std::thread m_thread;
        bool m_done;
        bool m_incoming;
        bool m_ready_for_first_send;
        std::list<std::vector<unsigned char> > m_msgs;
        std::vector<unsigned char> m_processing_msg;
    };

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
        assert(m_nodes.empty());
        assert(m_disconnected_nodes.empty());
    }

    void Done(ConnID id)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it(m_disconnected_nodes.find(id));
            m_disconnected_nodes.erase(it);
        }
        m_condvar.notify_one();
    }

protected:
    std::list<CConnection> OnNeedOutgoingConnections(int need_count) final
    {
        std::list<CConnection> ret;
        auto it = m_connections.begin();
        if (!m_connections.empty())
        {
            std::advance(it, std::min((size_t)need_count, m_connections.size()));
            ret.splice(ret.end(), m_connections, m_connections.begin(), it);
        }
        return ret;
    }
    CNodeEvents* OnOutgoingConnection(ConnID id, const CConnection& conn, const CConnection& resolved_conn) final
    {
        std::unique_ptr<CNode> ptr(new CNode(*this, id, resolved_conn, false));
        auto out(m_nodes.emplace_hint(m_nodes.end(), id, std::move(ptr)));
        printf("connected\n");

        if(m_nodes.size() >= 3)
            Shutdown();

        return out->second.get();
    }
    void OnShutdown() final
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condvar.wait(lock, [this]{return m_disconnected_nodes.empty();});
        }
        printf("all done\n");
        m_connections.clear();
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
        CNetworkConfig testnet_config;
        testnet_config.header_msg_string_offset = 4;
        testnet_config.header_msg_size_offset = 16;
        testnet_config.header_msg_size_size = 4;
        testnet_config.header_msg_string_size = 12;
        testnet_config.header_size = 24;
        testnet_config.message_max_size = 1000000 + testnet_config.header_size;
        testnet_config.message_start = {0x0b, 0x11, 0x09, 0x07};
        testnet_config.protocol_version = 70011;
        testnet_config.protocol_handshake_version = 209;
        testnet_config.service_flags = 0;

        /* Establish the connection options. See libbtcnet/connection.h for more. */
        CConnectionOptions myopts;
        myopts.nRetries = 0;
        myopts.doResolve = CConnectionOptions::RESOLVE_ONLY;
        myopts.nConnTimeout = 1;

        m_connections.emplace_back(myopts, testnet_config, "testnet-seed.bitcoin.petertodd.org", 8333);
        m_connections.emplace_back(myopts, testnet_config, "testnet-seed.bluematt.me", 8333);
        m_connections.emplace_back(myopts, testnet_config, "testnet-seed.bitcoin.schildbach.de", 8333);
    }

    CNodeEvents* OnIncomingConnection(ConnID id, const CConnection& listenconn, const CConnection& resolved_conn) final
    {
        std::unique_ptr<CNode> ptr(new CNode(*this, id, resolved_conn, true));
        auto out(m_nodes.emplace_hint(m_nodes.end(), id, std::move(ptr)));
        return out->second.get();
    }
    void OnDnsResponse(const CConnection& conn, std::list<CConnection> results) final
    {
        printf("added %lu addresses from dns seed\n", results.size());
        m_connections.splice(m_connections.end(), results);
    }
    bool OnConnectionFailure(const CConnection& conn, const CConnection& resolved, bool retry) final { return true; }
    bool OnDisconnected(ConnID id, bool persistent) final
    {
        auto it(m_nodes.find(id));
        auto node(std::move(it->second));
        m_nodes.erase(it);
        decltype(m_disconnected_nodes)::iterator newit;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            newit = m_disconnected_nodes.emplace_hint(m_disconnected_nodes.end(), id, std::move(node));
        }
        newit->second->Stop();
        return true;
    }
    void OnBindFailure(const CConnection& listener) final {}
    bool OnDnsFailure(const CConnection& conn, bool retry) final { return true; }
    bool OnProxyFailure(const CConnection& conn, bool retry) final { return true; }

private:
    std::list<CConnection> m_connections;
    std::map<ConnID, std::unique_ptr<CNode>> m_nodes;
    std::map<ConnID, std::unique_ptr<CNode>> m_disconnected_nodes;
    std::mutex m_mutex;
    std::condition_variable m_condvar;
};

int main()
{
    CConnman test(false);
    test.Run(16);
}
