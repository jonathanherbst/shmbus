#pragma once

#include "bus.hpp"

class bus_consumer
{
public:
    bus_consumer(bus::create, const std::string& name, size_t size_exponent);

    bus_consumer(bus::open, const std::string& name);

    void wait(const boost::posix_time::ptime& timeout);

    std::pair<const void*, size_t> data() const;

    void consume(size_t s);

    unsigned int read_offset() const;

    const bus& get_bus() const;

private:
    bus m_bus;
    size_t m_read_offset;
};
