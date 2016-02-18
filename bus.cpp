#include "bus.hpp"
#include "byte.hpp"

using namespace boost::interprocess;

std::string bus::mutex_name(const std::string& shmName)
{
    return shmName + "_mutex";
}

std::string bus::condition_name(const std::string& shmName)
{
    return shmName + "_condition";
}

const boost::interprocess::offset_t bus::data_offset = mapped_region::get_page_size();

bus::bus(create, const std::string& name, size_t sizeExponent) :
m_mutex(open_or_create, mutex_name(name).c_str()),
m_condition(open_or_create, condition_name(name).c_str()),
m_mem(open_or_create, name.c_str(), read_write)
{
    size_t size = 2 << sizeExponent;
    offset_t sizeRequested = size + mapped_region::get_page_size();
    m_mask = size - 1;
    scoped_lock<mutex_type> lock(m_mutex);
    offset_t memorySize;
    if(not m_mem.get_size(memorySize) or memorySize == 0)
        m_mem.truncate(sizeRequested);
    m_region = mapped_region(m_mem, read_write);
    m_write_offset = static_cast<size_t*>(m_region.get_address());
}

bus::bus(open, const std::string& name):
m_mutex(open_only, mutex_name(name).c_str()),
m_condition(open_only, condition_name(name).c_str()),
m_mem(open_only, name.c_str(), read_write),
m_region(m_mem, read_write),
m_mask(m_region.get_size() - data_offset - 1),
m_write_offset(static_cast<size_t*>(m_region.get_address()))
{}

bus::mutex_type& bus::mutex()
{
    return m_mutex;
}

bus::condition_type& bus::condition()
{
    return m_condition;
}

boost::interprocess::offset_t bus::size() const
{
    return m_region.get_size() - data_offset;
}

size_t bus::mask() const
{
    return m_mask;
}

void* bus::write_ptr()
{
    return byte_next(buf_begin(), (*m_write_offset) & m_mask);
}

void bus::commit(size_t bytes)
{
    *m_write_offset += bytes;
}

size_t bus::write_offset() const
{
    return *m_write_offset;
}

void* bus::buf_begin() const
{
    return byte_next(m_region.get_address(), data_offset);
}

void* bus::buf_end() const
{
    return byte_next(buf_begin(), size());
}
