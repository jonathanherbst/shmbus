#pragma once

#include "bus.hpp"

class bus_producer
{
public:
    bus_producer(bus::create, const std::string& name, size_t size_exponent);

    bus_producer(bus::open, const std::string& name);

    void write(const void* data, size_t len);

    const bus& get_bus() const;

private:
    bus m_bus;
};

