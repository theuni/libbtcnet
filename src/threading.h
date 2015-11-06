// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_THREADING_H
#define BITCOIN_NET_THREADING_H

#ifndef NO_THREADS

#ifdef USE_CXX11
#include <mutex>
#include <condition_variable>
#include <thread>
typedef std::mutex base_mutex_type;
typedef std::lock_guard<base_mutex_type> lock_guard_type;
typedef std::unique_lock<base_mutex_type> unique_lock_type;
typedef std::condition_variable condition_variable_type;
typedef std::thread thread_type;
#else
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
typedef boost::mutex base_mutex_type;
typedef boost::lock_guard<base_mutex_type> lock_guard_type;
typedef boost::unique_lock<base_mutex_type> unique_lock_type;
typedef boost::condition_variable condition_variable_type;
#endif

class mutex_type
{
public:
    inline void lock()
    {
        m_mutex.lock();
    }
    inline void unlock()
    {
        m_mutex.unlock();
    }
private:
    base_mutex_type m_mutex;
};

class optional_lock_type
{
public:
    optional_lock_type(mutex_type* mutex, bool do_lock)
       : m_mutex(mutex), m_do_lock(do_lock)
    {
       if(m_mutex && do_lock)
           m_mutex->lock();
    }

    ~optional_lock_type()
    {
       if(m_mutex && m_do_lock)
           m_mutex->unlock();
    }
private:
    mutex_type* m_mutex;
    const bool m_do_lock;
};

#endif
#endif
