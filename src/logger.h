// Copyright (c) 2016 Cory Fields
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LIBBTCNET_SRC_LOGGER_H
#define LIBBTCNET_SRC_LOGGER_H
#include <iostream>

#if !defined(LOGLEVEL)
#define LOGLEVEL LOGERR
#endif

namespace logger
{
enum loglevel {
    LOGNONE,
    LOGERR,
    LOGWARN,
    LOGINFO,
    LOGVERBOSE,
    LOGALL
};

template <loglevel l>
struct mylogger {
    static inline void log()
    {
        std::cout << std::endl;
    }

    template <typename First, typename... Rest>
    static inline void log(First&& first, Rest&&... rest)
    {
        if (LOGLEVEL >= l) {
            std::cout << std::forward<First>(first) << " ";
            mylogger::log(std::forward<Rest>(rest)...);
        }
    }
};
} // namespace logger
#ifndef NDEBUG
#define DEBUG_PRINT(MYLEVEL, ...) logger::mylogger<logger::loglevel::MYLEVEL>::log(__func__, __VA_ARGS__);
#else
#define DEBUG_PRINT(...)
#endif

#endif
