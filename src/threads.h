// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCNET_THREADS_H
#define BTCNET_THREADS_H

#include "eventtypes.h"

#ifndef NO_THREADS
#include <mutex>
#include <event2/thread.h>
#endif

#include <event2/event.h>
#include <stdexcept>

#ifndef NO_THREADS

class optional_lock_type
{
public:
    optional_lock_type(std::mutex& mutex, bool do_lock)
        : m_mutex(mutex), m_do_lock(do_lock)
    {
        if (do_lock)
            m_mutex.lock();
    }

    ~optional_lock_type()
    {
        if (m_do_lock)
            m_mutex.unlock();
    }

private:
    std::mutex& m_mutex;
    const bool m_do_lock;
};
#endif

#ifndef NO_THREADS
#define optional_lock(mutex, enabled) optional_lock_type mylock(mutex, enabled)
#else
#define optional_lock(mutex, enabled)
#endif

static inline void setup_threads()
{
#if defined(NO_THREADS)
    throw std::runtime_error("Thread support requested but not compiled in.");
#else
#if defined(EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED)
    evthread_use_windows_threads();
    return;
#elif defined(EVTHREAD_USE_PTHREADS_IMPLEMENTED)
    evthread_use_pthreads();
    return;
#endif
#endif
}

static inline int enable_threads_for_handler(const event_type<event_base>& base)
{
#if !defined(_EVENT_DISABLE_THREAD_SUPPORT) && !defined(NO_THREADS)
    return evthread_make_base_notifiable(base);
#else
    return 0;
#endif
}

#endif
