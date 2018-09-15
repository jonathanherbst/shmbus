#include "consumer.hpp"
#include "util.hpp"

namespace {

}

namespace shmbus {

consumer::consumer(detail::create_ m, const std::string& name, size_t size_exponent) :
m_bus(m, name, size_exponent),
m_read_index(m_bus.read_index())
{}

consumer::consumer(detail::open_ m, const std::string& name) :
m_bus(m, name),
m_read_index(m_bus.read_index())
{}

void consumer::wait(const boost::posix_time::ptime& timeout)
{
    detail::fake_mutex m;
    boost::interprocess::scoped_lock<detail::fake_mutex> l(m);
    m_bus.condition().timed_wait(l, timeout);
}

std::pair<const void*, std::size_t> consumer::data() const
{
    return m_bus.read_buffer(m_read_index);
}

void consumer::consume(std::size_t s)
{
    m_bus.consume_read_index(m_read_index, s);
}

unsigned int consumer::read_index() const
{
    return m_read_index;
}

const bus& consumer::get_bus() const
{
    return m_bus;
}

}
