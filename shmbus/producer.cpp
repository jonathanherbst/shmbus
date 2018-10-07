#include "producer.hpp"

#include <boost/interprocess/sync/scoped_lock.hpp>

#include <cassert>

namespace shmbus {

producer::producer(detail::create_ m, const std::string& name, std::size_t size_exponent) :
m_bus(m, name, size_exponent)
{}

producer::producer(detail::open_ m, const std::string& name) :
m_bus(m, name)
{}

std::size_t producer::write_some(const void* data, std::size_t len)
{
    len = std::min(len, m_bus.capacity());
    boost::interprocess::scoped_lock<bus::mutex_type> lock(m_bus.mutex());
    void* buffer;
    std::size_t buffer_size;
    std::tie(buffer, buffer_size) = m_bus.write_buffer();

    const uint8_t* data_ptr = static_cast<const uint8_t*>(data);
    size_t write_size = std::min(len, buffer_size);
    memcpy(buffer, data_ptr, write_size);
    m_bus.commit(write_size);

    if(write_size < len)
    { // didn't write everything, either because the buffer wrapped or we are hitting a mandatory consumer
        std::tie(buffer, buffer_size) = m_bus.write_buffer();
        size_t write_size2 = std::min(len - write_size, buffer_size);
        memcpy(buffer, data_ptr + write_size, write_size2);
        m_bus.commit(write_size2);
        write_size += write_size2;
    }

    return write_size;
}

const bus& producer::get_bus() const
{
    return m_bus;
}

}
