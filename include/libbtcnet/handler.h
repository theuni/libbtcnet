// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_HANDLER_H
#define LIBBTCNET_HANDLER_H

#include <list>
#include <vector>
#include <stddef.h>
#include <stdint.h>

class CConnection;
class CRateLimit;

struct CNodeMessages {
    std::list<std::vector<unsigned char> > messages;
    size_t size;
};

typedef int64_t ConnID;

class CConnectionHandlerInt;
struct CNetworkConfig;

class CConnectionHandler
{
    friend class CConnectionHandlerInt;
    friend class ConnectionBase;

public:
    /// \brief Disconnect the remote
    ///
    /// Optionally wait for an empty send buffer. This is useful for sending a
    /// termination message. While waiting for the send buffer to drain, no new
    /// incoming data will be read.
    /// \param id The connection's unique id
    /// \param immediately Whether to wait for an empty send buffer
    void CloseConnection(ConnID id, bool immediately);

    /// \brief Send data to the remote
    ///
    /// \param id The connection's unique id
    /// \param data Raw data to send
    /// \param size Size of the raw data
    bool Send(ConnID id, const unsigned char* data, size_t size);

    /// \brief Set a rate limit for a specific connection
    ///
    /// This may only be called after the handler has been started. See OnStartup.
    void SetRateLimit(ConnID id, const CRateLimit& limit);

    /// \brief Pause receiving data from the remote
    /// This may only be called after the handler has been started. See OnStartup.
    /// \param id The connection's unique id
    void PauseRecv(ConnID id);

    /// \brief Resume receiving data from the remote
    /// This may only be called after the handler has been started. See OnStartup.
    /// \param id The connection's unique id
    void UnpauseRecv(ConnID id);

    /// \brief Reset the ping timeout
    ///
    /// This may only be called after the handler has been started. See OnStartup.
    /// Once bound, new connection notifications will come via OnIncomingConnection
    /// \param id The connection's unique id
    /// \param seconds new timeout value, or 0 to disable
    void ResetPingTimeout(ConnID id, int seconds);

    /// \brief Begin the shutdown process
    ///
    /// This may only be called after the handler has been started. See OnStartup.
    /// Cancels all existing connection attempts, tears down established
    /// connections, and unbinds all bound addresses. All necessary callbacks will
    /// be called, so that implementors may clean up as necessary. Once finished,
    /// OnShutdown is called.
    /// Note that PumpEvents must continue to be called after calling Shutdown()
    /// until OnShutdown has been received, and/or PumpEvents returns false.
    void Shutdown();

    /// \brief Set a rate limit for all incoming connections combined
    ///
    /// This may only be called after the handler has been started. See OnStartup.
    /// Sets a collective rate limit for incoming connections. Individual
    /// connections will be capped by this as well if their limits are higher.
    /// \param limit The new incoming rate limit
    void SetIncomingRateLimit(const CRateLimit& limit);

    /// \brief Set a rate limit for all outgoing connections combined
    ///
    /// This may only be called after the handler has been started. See OnStartup.
    /// Sets a collective rate limit for outgoing connections. Individual
    /// connections will be capped by this as well if their limits are higher.
    /// \param limit The new outgoing rate limit
    void SetOutgoingRateLimit(const CRateLimit& limit);

    /// \brief Bind an address and listen for new connections on it
    ///
    /// This may only be called after the handler has been started. See OnStartup.
    /// Once bound, new connection notifications will come via OnIncomingConnection
    /// \returns true if the address was successfully bound
    bool Bind(const CConnection& conn);

protected:
    /// \brief Create a Connection Handler
    ///
    /// Handlers may be used multi-threaded or single-threaded. If all calls into
    /// the handler will be coming from the same thread, consider setting
    /// enable_threading to false to avoid locking overhead.
    ///
    /// \param enable_threading Whether the handler should be threadsafe

    explicit CConnectionHandler(bool enable_threading);

    virtual ~CConnectionHandler();

    /// \brief Pump the handler for new events
    ///
    /// This must only be called from a single thread, and only after Start() has
    /// been called. It should be called until it returns false in order to ensure
    /// that all events (including the shutdown event) have been processed.
    ///
    /// In threaded mode, this will typically be called in a tight loop:
    /// while(PumpEvents(true))
    ///
    /// In non-threaded mode, the block param may be set to false in order to check
    /// for new events in multiple places without stalling the main loop:
    /// while(PumpEvents(true))
    /// {
    ///     // assuming msgs is populated by OnReceiveMessages
    ///     for(auto& msg : msgs)
    ///     {
    ///         Process(msg)
    ///         PumpEvents(false);
    ///     }
    /// }
    ///
    /// \param block Whether the call should block while waiting for a new event.
    ///
    /// \returns true while there are still events to process
    bool PumpEvents(bool block);

    /// \brief Start the connection handler
    ///
    /// This must be called by the thread that will be calling PumpEvents.
    ///
    /// \param outgoing_limit Target number of outgoing connections
    void Start(int outgoing_limit);

    /// \brief Request for outgoing connections
    ///
    /// When the handler does not have enough current outgoing connections, this
    /// will be called to request more. It expects to receive a list of candidate
    /// connections, up to needcount. If more than needcount candidates are
    /// provided, the excess will be ignored.
    ///
    /// \param needcount The maximum available connection slots for this request
    /// \returns A list of outgoing connections
    virtual std::list<CConnection> OnNeedOutgoingConnections(int needcount) = 0;


    /// \brief Notification of Bind failure
    ///
    /// Called when an established bind fails for some reason
    /// \param bind The bound address
    virtual void OnBindFailure(const CConnection& bind) = 0;

    /// \brief Notification of DNS query response
    ///
    /// Called when a resolve-only request has succeeded.
    /// \param conn The original request
    /// \param results A list of resolved addresses
    virtual void OnDnsResponse(const CConnection& conn, std::list<CConnection> results) = 0;

    /// \brief Notification of DNS query failure
    ///
    /// Called when a resolve-only request has failed.
    /// \param conn The original request
    /// \param retry Whether or not the request will be retried
    virtual bool OnDnsFailure(const CConnection& conn, bool retry) = 0;

    /// \brief Notification of outgoing connection failure
    ///
    /// Called when an outgoing connection has failed.
    /// \param conn The original address
    /// \param resolved The attempted resolved address if applicable, otherwise
    ///        the original address again.
    /// \param retry Whether or not the connection will be retried
    virtual bool OnConnectionFailure(const CConnection& conn, const CConnection& resolved, bool retry) = 0;

    /// \brief Notification of outgoing proxy failure
    ///
    /// Called when an outgoing proxy connection has failed.
    /// \param conn The original address
    /// \param retry Whether or not the connection will be retried
    virtual bool OnProxyFailure(const CConnection& conn, bool retry) = 0;

    /// \brief Notification of a successful outgoing connection
    ///
    /// Called when an outgoing connection has succeeded.
    /// \param id A unique id that will be used to reference this connection
    /// \param conn The original address
    /// \param resolved The attempted resolved address if applicable, otherwise
    ///        the original address again.
    /// \returns false if the new connection is unwanted and should be immediately
    ///          disconnected, otherwise true.
    virtual bool OnOutgoingConnection(ConnID id, const CConnection& conn, const CConnection& resolved) = 0;

    /// \brief Notification of a successful outgoing connection
    ///
    /// Called when an outgoing connection has succeeded.
    /// \param id A unique id that will be used to reference this connection
    /// \param bind The original bind address
    /// \param resolved The incoming client's address
    /// \returns false if the new connection is unwanted and should be immediately
    ///          disconnected, otherwise true.
    virtual bool OnIncomingConnection(ConnID id, const CConnection& bind, const CConnection& resolved) = 0;

    /// \brief Notification of a disconnected connection
    ///
    /// Called when an existing connection is disconnected
    /// \param id The connection's unique id
    /// \param persistent Whether or not a reconnection attempt will be made
    virtual bool OnDisconnected(ConnID id, bool persistent) = 0;

    /// \brief Notification of a outgoing readyness
    ///
    /// Called when an outgoing connection has been established and is waiting on
    /// its first send.
    /// \param id The connection's unique id
    virtual void OnReadyForFirstSend(ConnID id) = 0;

    /// \brief Notification of new messages
    ///
    /// Called when at least one complete message has been received
    /// \param id The connection's unique id
    /// \param msgs A list of received messages
    /// \param totalsize The length of all combined messages
    virtual bool OnReceiveMessages(ConnID id, std::list<std::vector<unsigned char> > msgs, size_t totalsize) = 0;

    /// \brief Notification of a malformed message
    ///
    /// Called when a message is received with a corrupt or incorrect header
    /// This will be followed by an OnDisconnected event.
    /// \param id The connection's unique id
    virtual void OnMalformedMessage(ConnID id) = 0;

    /// \brief Notification of a full write buffer
    ///
    /// Called when a connection's write buffer has exceeded its allowed size.
    /// \param id The connection's unique id
    /// \param bufsize The current write buffer size
    virtual void OnWriteBufferFull(ConnID id, size_t bufsize) = 0;

    /// \brief Notification of a no-longer-full write buffer
    ///
    /// Called when a connection's write buffer was full, and is now below the
    /// maximum size.
    /// \param id The connection's unique id
    /// \param bufsize The current write buffer size
    virtual void OnWriteBufferReady(ConnID id, size_t bufsize) = 0;

    /// \brief Notification of bytes read from the remote connection
    ///
    /// Called every time a chunk is read from the remote. Does not indicate a
    /// complete message has been received.
    /// \param id The connection's unique id
    /// \param bytes Number of bytes read since the last notification
    /// \param total_bytes Total bytes read
    virtual void OnBytesRead(ConnID id, size_t bytes, size_t total_bytes) = 0;

    /// \brief Notification of bytes written to the remote connection
    ///
    /// Called every time a chunk is written to the remote. Does not indicate a
    /// complete message has been sent.
    /// \param id The connection's unique id
    /// \param bytes Number of bytes written since the last notification
    /// \param total_bytes Total bytes written
    virtual void OnBytesWritten(ConnID id, size_t bytes, size_t total_bytes) = 0;

    /// \brief Notification of ping timeout
    ///
    /// Called when the ping timer reaches zero before it is reset
    /// \param id The connection's unique id
    virtual void OnPingTimeout(ConnID id) = 0;

    /// \brief Notification of shutdown of the manager
    ///
    /// This will always be the last event to be sent. It indicates that all events
    /// have been processed, connections have been torn down, and binds have been
    /// unbound.
    virtual void OnShutdown() = 0;

    /// \brief Notification of a successful manager startup
    ///
    /// This will always be the first event to be sent. It is a convenience
    /// function that can be used to establish binds, rate limits, etc.
    virtual void OnStartup() = 0;

private:
    CConnectionHandlerInt* m_internal;
};

#endif // LIBBTCNET_HANDLER_H
