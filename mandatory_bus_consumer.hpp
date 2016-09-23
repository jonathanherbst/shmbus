#pragma once

#include "bus.hpp"

class mandatory_bus_consumer
{
public:
    typedef unsigned int size_type;
    typedef unsigned int offset_type;

    mandatory_bus_consumer(bus::create, const std::string& name, size_type size_exponent);

    mandatory_bus_consumer(bus::open, const std::string& name);

    void wait(const boost::posix_time::ptime& timeout);

    std::pair<const void*, size_type> data() const;

    void consume(size_type s);

    offset_type read_offset() const;

    const bus& get_bus() const;

private:
    bus m_bus;
    offset_type m_read_offset;
};
