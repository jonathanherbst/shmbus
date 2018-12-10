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
    void* buffer1;
    std::size_t buffer_size1;
    void* buffer2;
    std::size_t buffer_size2;
    std::tie(buffer1, buffer_size1, buffer2, buffer_size2) = m_bus.write_buffer();

    if(buffer_size1 + buffer_size2 < len)
    {
        // notify to wake up consumers
        m_bus.condition().notify_all();
        return 0;
    }

    const uint8_t* data_ptr = static_cast<const uint8_t*>(data);
    size_t write_size = std::min(len, buffer_size1);
    memcpy(buffer1, data_ptr, write_size);
    memcpy(buffer2, data_ptr + write_size, len - write_size);
    m_bus.commit(len);

    m_bus.condition().notify_all();

    return len;
}

const bus& producer::get_bus() const
{
    return m_bus;
}

}
