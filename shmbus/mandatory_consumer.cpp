#include "mandatory_consumer.hpp"
#include "errors.hpp"
#include "util.hpp"

#include <boost/thread/thread_time.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace shmbus {

mandatory_consumer::mandatory_consumer(detail::create_ m, const std::string& bus_name, std::size_t size_exponent, uint8_t id) :
m_bus(m, bus_name, size_exponent)
{
    boost::interprocess::scoped_lock<bus::mutex_type> lock(m_bus.mutex());
    m_data = m_bus.open_mandatory_consumer(id);
    if(not m_data)
        throw out_of_mandatory_consumers();
}

mandatory_consumer::mandatory_consumer(detail::open_ m, const std::string& bus_name, uint8_t id):
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

void mandatory_consumer::wait_for(const std::chrono::microseconds& timeout)
{
    wait(boost::get_system_time() + boost::posix_time::microseconds(timeout.count()));
}

void mandatory_consumer::wait_for_data(const std::chrono::microseconds& timeout)
{
    boost::posix_time::ptime wait_time(boost::get_system_time() +
        boost::posix_time::microseconds(timeout.count()));
    boost::interprocess::sharable_lock<bus::mutex_type> l(m_bus.mutex());
    if(data().second > 0)
        return;
    m_bus.condition().timed_wait(l, wait_time);
}

std::pair<const void*, std::size_t> mandatory_consumer::data() const
{
    return m_bus.read_buffer(m_data->read_index);
}

void mandatory_consumer::consume(std::size_t s)
{
    m_data->read_index = m_bus.consume_read_index(m_data->read_index, s);
}

const bus& mandatory_consumer::get_bus() const
{
    return m_bus;
}

}