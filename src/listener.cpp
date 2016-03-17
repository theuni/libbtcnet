// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "listener.h"
#include "incomingconn.h"
#include "eventtypes.h"

#include <string.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <assert.h>

CConnListener::CConnListener(CConnectionHandlerInt& handler, const event_type<event_base>& base, ConnID id, CConnection conn)
    : m_handler(handler), m_event_base(base), m_id(id), m_connection(std::move(conn))
{
}

CConnListener::~CConnListener()
{
    Unbind();
}

const CConnection& CConnListener::GetListenConnection() const
{
    return m_connection;
}


bool CConnListener::Bind()
{
    assert(!m_listener);
    assert(m_event_base != nullptr);

    sockaddr_storage addr_storage;
    memset(&addr_storage, 0, sizeof(addr_storage));
    int socklen = sizeof(addr_storage);
    sockaddr* addr = reinterpret_cast<sockaddr*>(&addr_storage);
    m_connection.GetSockAddr(addr, &socklen);

    m_listener.reset(evconnlistener_new_bind(m_event_base, accept_conn, this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, addr, socklen));
    if (!m_listener) {
        m_handler.OnListenFailure(m_id, m_connection);
        return false;
    }
    return true;
}

bool CConnListener::Enable()
{
    assert(m_listener);
    return evconnlistener_enable(m_listener) == 0;
}

bool CConnListener::Disable()
{
    assert(m_listener);
    return evconnlistener_disable(m_listener) == 0;
}

void CConnListener::Unbind()
{
    m_listener.free();
}

void CConnListener::listen_error_cb(evconnlistener* /*unused*/, void* ctx)
{
    assert(ctx != nullptr);
    CConnListener* bind = static_cast<CConnListener*>(ctx);
    bind->m_listener.free();
    bind->m_handler.OnListenFailure(bind->m_id, bind->m_connection);
}

void CConnListener::accept_conn(evconnlistener* /*unused*/, evutil_socket_t fd, sockaddr* address, int socklen, void* ctx)
{
    assert(ctx != nullptr);
    CConnListener* bind = static_cast<CConnListener*>(ctx);
    bind->m_handler.OnIncomingConnection(bind->m_connection, fd, address, socklen);
}
