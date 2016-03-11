// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_THREADS_H
#define LIBBTCNET_SRC_THREADS_H

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

static inline bool setup_threads()
{
#if defined(NO_THREADS)
    return false;
#else
#if defined(EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED)
    return evthread_use_windows_threads() == 0;
#elif defined(EVTHREAD_USE_PTHREADS_IMPLEMENTED)
    return evthread_use_pthreads() == 0;
#endif
#endif
}

static inline int enable_threads_for_handler(const event_type<event_base>& base)
{
#if !defined(_EVENT_DISABLE_THREAD_SUPPORT) && !defined(NO_THREADS)
    return evthread_make_base_notifiable(base);
#else
    (void)base;
    return 0;
#endif
}

#endif
