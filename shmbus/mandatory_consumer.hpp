#pragma once

#include "bus.hpp"

#include <chrono>

namespace shmbus {

class mandatory_consumer
{
public:
    mandatory_consumer(detail::create_, const std::string& bus_name, std::size_t size_exponent, const uint8_t* id);

    mandatory_consumer(detail::open_, const std::string& bus_name, const uint8_t* id);

    mandatory_consumer(const mandatory_consumer&) = delete;

    ~mandatory_consumer();

    mandatory_consumer& operator=(const mandatory_consumer&) = delete;

    void wait(const boost::posix_time::ptime& timeout);

    void wait_for(const std::chrono::microseconds& timeout);

    std::pair<const void*, std::size_t> data() const;

    void consume(std::size_t s);

    const bus& get_bus() const;

private:
    bus m_bus;
    detail::mandatory_consumer_data* m_data;
};

}
