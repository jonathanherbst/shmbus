#include "consumer.hpp"
#include "util.hpp"

#include <boost/thread/thread_time.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

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

void consumer::wait_for(const std::chrono::microseconds& timeout)
{
    wait(boost::get_system_time() + boost::posix_time::microseconds(timeout.count()));
}

void consumer::wait_for_data(const std::chrono::microseconds& timeout)
{
    boost::posix_time::ptime wait_time(boost::get_system_time() +
        boost::posix_time::microseconds(timeout.count()));
    boost::interprocess::scoped_lock<bus::mutex_type> l(m_bus.mutex());
    if(data().second > 0)
        return;
    m_bus.condition().timed_wait(l, wait_time);
}

std::pair<const void*, std::size_t> consumer::data() const
{
    return m_bus.read_buffer(m_read_index);
}

void consumer::consume(std::size_t s)
{
    m_read_index = m_bus.consume_read_index(m_read_index, s);
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
