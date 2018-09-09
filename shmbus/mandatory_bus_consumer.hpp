#pragma once

#include "bus.hpp"

namespace shmbus {

class mandatory_bus_consumer
{
public:
    mandatory_bus_consumer(detail::create_, const std::string& bus_name, std::size_t size_exponent, const uint8_t* id);

    mandatory_bus_consumer(detail::open_, const std::string& bus_name, const uint8_t* id);

    mandatory_bus_consumer(const mandatory_bus_consumer&) = delete;

    ~mandatory_bus_consumer();

    mandatory_bus_consumer& operator=(const mandatory_bus_consumer&) = delete;

    void wait(const boost::posix_time::ptime& timeout);

    std::pair<const void*, std::size_t> data() const;

    void consume(std::size_t s);

    const bus& get_bus() const;

private:
    bus m_bus;
    detail::mandatory_consumer_data* m_data;
};

}
