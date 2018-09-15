#pragma once

#include "boost/date_time/posix_time/posix_time_types.hpp"

namespace shmbus {
namespace detail {

struct fake_mutex
{
    struct internal_mutex_type {};

    void lock() {}
    bool try_lock() { return true; }
    bool timed_lock(const boost::posix_time::ptime&) { return true; }
    void unlock() {}
};

template<typename T>
T pow2RoundDown(T v)
{
    for(T i = 1; (v & ~i) > 0; i <<= 1)
        v &= ~i;
    return v;
}

}
}