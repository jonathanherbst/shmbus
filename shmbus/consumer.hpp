#pragma once

#include "bus.hpp"

#include <chrono>

namespace shmbus {

class consumer
{
public:
    consumer(detail::create_, const std::string& name, std::size_t size_exponent);

    consumer(detail::open_, const std::string& name);

    void wait(const boost::posix_time::ptime& timeout);

    void wait_for(const std::chrono::microseconds& timeout);

    std::pair<const void*, std::size_t> data() const;

    void consume(std::size_t s);

    unsigned int read_index() const;

    const bus& get_bus() const;

private:
    bus m_bus;
    std::size_t m_read_index;
};

}
