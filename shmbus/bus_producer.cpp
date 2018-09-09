#include "bus_producer.hpp"
#include "byte.hpp"

#include <cassert>

namespace shmbus {

bus_producer::bus_producer(detail::create_ m, const std::string& name, std::size_t size_exponent) :
m_bus(m, name, size_exponent)
{}

bus_producer::bus_producer(detail::open_ m, const std::string& name) :
m_bus(m, name)
{}

std::size_t bus_producer::write_some(const void* data, std::size_t len)
{
    assert(len < m_bus.size());

    boost::interprocess::scoped_lock<bus::mutex_type> lock(m_bus.mutex());
    volatile void* buffer;
    std::size_t buffer_size;
    std::tie(buffer, buffer_size) = m_bus.write_buffer();

    const uint8_t* data_ptr = static_cast<const uint8_t*>(data);
    size_t write_size = std::min(len, buffer_size);
    std::copy(data_ptr, data_ptr + write_size, static_cast<volatile uint8_t*>(buffer));
    m_bus.commit(write_size);

    if(write_size < len)
    { // didn't write everything, either because the buffer wrapped or we are hitting a mandatory consumer
        std::tie(buffer, buffer_size) = m_bus.write_buffer();
        size_t write_size2 = std::min(len - write_size, buffer_size);
        std::copy(data_ptr + write_size, data_ptr + write_size + write_size2, static_cast<volatile uint8_t*>(buffer));
        m_bus.commit(write_size2);
        write_size += write_size2;
    }

    return write_size;
}

const bus& bus_producer::get_bus() const
{
    return m_bus;
}

}
