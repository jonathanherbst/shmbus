#pragma once

#include "bus.hpp"

namespace shmbus {

class bus_producer
{
public:
    bus_producer(detail::create_, const std::string& name, std::size_t size_exponent);

    bus_producer(detail::open_, const std::string& name);

    std::size_t write_some(const void* data, size_t len);

    const bus& get_bus() const;

private:
    bus m_bus;
};

}
