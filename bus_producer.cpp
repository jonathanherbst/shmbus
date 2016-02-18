#include "bus_producer.hpp"
#include "byte.hpp"

bus_producer::bus_producer(bus::create m, const std::string& name, size_t size_exponent) :
m_bus(m, name, size_exponent)
{}

bus_producer::bus_producer(bus::open m, const std::string& name) :
m_bus(m, name)
{}

void bus_producer::write(const void* data, size_t len)
{
    boost::interprocess::scoped_lock<bus::mutex_type> lock(m_bus.mutex());
    size_t writeLeft = len;
    void* write_ptr = m_bus.write_ptr();
    size_t dataLeft = m_bus.size() - (m_bus.write_offset() & m_bus.mask());
    if(dataLeft < len)
    {
        memcpy(write_ptr, data, dataLeft);
        data = byte_next(data, dataLeft);
        writeLeft -= dataLeft;
        write_ptr = m_bus.buf_begin();
    }

    memcpy(write_ptr, data, writeLeft);
    m_bus.commit(len);
    m_bus.condition().notify_all();
}

const bus& bus_producer::get_bus() const
{
    return m_bus;
}
