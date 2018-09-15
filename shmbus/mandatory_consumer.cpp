#include "mandatory_consumer.hpp"
#include "errors.hpp"
#include "util.hpp"

namespace shmbus {

mandatory_consumer::mandatory_consumer(detail::create_ m, const std::string& bus_name, std::size_t size_exponent, const uint8_t* id) :
m_bus(m, bus_name, size_exponent)
{
    boost::interprocess::scoped_lock<bus::mutex_type> lock(m_bus.mutex());
    m_data = m_bus.open_mandatory_consumer(id);
    if(not m_data)
        throw out_of_mandatory_consumers();
}

mandatory_consumer::mandatory_consumer(detail::open_ m, const std::string& bus_name, const uint8_t* id):
m_bus(m, bus_name)
{
    boost::interprocess::scoped_lock<bus::mutex_type> lock(m_bus.mutex());
    m_data = m_bus.open_mandatory_consumer(id);
    if(not m_data)
        throw out_of_mandatory_consumers();
}

mandatory_consumer::~mandatory_consumer()
{
    boost::interprocess::scoped_lock<bus::mutex_type> lock(m_bus.mutex());
    m_bus.close_mandatory_consumer(m_data);
}

void mandatory_consumer::wait(const boost::posix_time::ptime& timeout)
{
    detail::fake_mutex m;
    boost::interprocess::scoped_lock<detail::fake_mutex> l(m);
    m_bus.condition().timed_wait(l, timeout);
}

std::pair<const void*, std::size_t> mandatory_consumer::data() const
{
    return m_bus.read_buffer(m_data->read_index);
}

void mandatory_consumer::consume(std::size_t s)
{
    m_bus.consume_read_index(m_data->read_index, s);
}

const bus& mandatory_consumer::get_bus() const
{
    return m_bus;
}

}