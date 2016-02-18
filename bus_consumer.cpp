#include "bus_consumer.hpp"
#include "byte.hpp"

namespace {

struct fake_mutex
{
    struct internal_mutex_type {};

    void lock() {}
    bool try_lock() { return true; }
    bool timed_lock(const boost::posix_time::ptime&) { return true; }
    void unlock() {}
};

}

bus_consumer::bus_consumer(bus::create m, const std::string& name, size_t size_exponent) :
m_bus(m, name, size_exponent),
m_read_offset(m_bus.write_offset())
{}

bus_consumer::bus_consumer(bus::open m, const std::string& name) :
m_bus(m, name),
m_read_offset(m_bus.write_offset())
{}

void bus_consumer::wait(const boost::posix_time::ptime& timeout)
{
    fake_mutex m;
    boost::interprocess::scoped_lock<fake_mutex> l(m);
    m_bus.condition().timed_wait(l, timeout);
}

std::pair<const void*, size_t> bus_consumer::data() const
{
    const void* readptr = byte_next(m_bus.buf_begin(), m_read_offset & m_bus.mask());
    size_t dataSize = std::min<size_t>(m_bus.write_offset() - m_read_offset,
        byte_diff(m_bus.buf_end(), readptr));
    return std::make_pair(readptr, dataSize);
}

void bus_consumer::consume(size_t s)
{
    assert(m_bus.write_offset() - m_read_offset >= s);

    m_read_offset += s;
}

unsigned int bus_consumer::read_offset() const
{
    return m_read_offset;
}

const bus& bus_consumer::get_bus() const
{
    return m_bus;
}
