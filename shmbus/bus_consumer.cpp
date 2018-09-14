#include "bus_consumer.hpp"
#include "util.hpp"

namespace {

}

namespace shmbus {

bus_consumer::bus_consumer(detail::create_ m, const std::string& name, size_t size_exponent) :
m_bus(m, name, size_exponent),
m_read_index(m_bus.read_index())
{}

bus_consumer::bus_consumer(detail::open_ m, const std::string& name) :
m_bus(m, name),
m_read_index(m_bus.read_index())
{}

void bus_consumer::wait(const boost::posix_time::ptime& timeout)
{
    detail::fake_mutex m;
    boost::interprocess::scoped_lock<detail::fake_mutex> l(m);
    m_bus.condition().timed_wait(l, timeout);
}

std::pair<const void*, std::size_t> bus_consumer::data() const
{
    return m_bus.read_buffer(m_read_index);
}

void bus_consumer::consume(std::size_t s)
{
    m_bus.consume_read_index(m_read_index, s);
}

unsigned int bus_consumer::read_index() const
{
    return m_read_index;
}

const bus& bus_consumer::get_bus() const
{
    return m_bus;
}

}
