#pragma once

#include "bus.hpp"

namespace shmbus {

class bus_consumer
{
public:
    bus_consumer(detail::create_, const std::string& name, std::size_t size_exponent);

    bus_consumer(detail::open_, const std::string& name);

    void wait(const boost::posix_time::ptime& timeout);

    std::pair<const void*, std::size_t> data() const;

    void consume(std::size_t s);

    unsigned int read_index() const;

    const bus& get_bus() const;

private:
    bus m_bus;
    std::size_t m_read_index;
};

}
