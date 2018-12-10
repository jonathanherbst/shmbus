#include "bus.hpp"
#include "errors.hpp"
#include "util.hpp"

#include <boost/interprocess/sync/scoped_lock.hpp>

#include <algorithm>

using namespace boost::interprocess;

namespace shmbus {

namespace detail {

mandatory_consumer_data::mandatory_consumer_data() :
id(0)
{}

mandatory_consumer_data::mandatory_consumer_data(uint8_t id, std::size_t read_index) :
id(id),
read_index(read_index)
{}

}

void bus::destroy(const std::string& name)
{
    mutex_type::remove(mutex_name(name).c_str());
    condition_type::remove(mutex_name(name).c_str());
    shared_memory_object::remove(name.c_str());
}

bus::bus(detail::create_, const std::string& name, size_t sizeExponent) :
m_mutex(open_or_create, mutex_name(name).c_str()),
m_condition(open_or_create, condition_name(name).c_str()),
m_mem(open_or_create, name.c_str(), read_write)
{
    size_t size = 2 << sizeExponent;
    offset_t sizeRequested = size + sizeof(detail::meta_page);
    scoped_lock<mutex_type> lock(m_mutex);
    offset_t memorySize;
    if(not m_mem.get_size(memorySize) or memorySize == 0)
        m_mem.truncate(sizeRequested);
    m_region = mapped_region(m_mem, read_write);
    m_meta = new(m_region.get_address()) volatile detail::meta_page;
    m_data = static_cast<uint8_t*>(m_region.get_address()) + sizeof(detail::meta_page);
    m_data_size = detail::pow2RoundDown(m_region.get_size() - sizeof(detail::meta_page));
    m_data_size_mask = m_data_size - 1;
}

bus::bus(detail::open_, const std::string& name):
m_mutex(open_only, mutex_name(name).c_str()),
m_condition(open_only, condition_name(name).c_str()),
m_mem(open_only, name.c_str(), read_write),
m_region(m_mem, read_write),
m_meta(static_cast<volatile detail::meta_page*>(m_region.get_address())),
m_data(static_cast<uint8_t*>(m_region.get_address()) + sizeof(detail::meta_page)),
m_data_size(detail::pow2RoundDown(m_region.get_size() - sizeof(detail::meta_page))),
m_data_size_mask(m_data_size - 1)
{}

std::size_t bus::capacity() const
{
    return m_data_size;
}

bus::mutex_type& bus::mutex()
{
    return m_mutex;
}

bus::condition_type& bus::condition()
{
    return m_condition;
}

std::tuple<void*, std::size_t, void*, std::size_t> bus::write_buffer()
{
    // find the max size of the writable section of the buffer
    std::size_t max_size = m_data_size;
    for(unsigned int i(0); i < m_meta->max_mandatory_consumers; ++i)
    {
        auto* consumer = m_meta->mandatory_consumers + i;
        if(consumer->id != 0)
        {
            std::size_t read_index = consumer->read_index;
            if(read_index == 0)
                max_size = std::min(max_size, m_data_size - m_meta->write_index - 1);
            else if(read_index > m_meta->write_index)
                max_size = std::min(max_size, read_index - m_meta->write_index - 1);
            else
                max_size = std::min(max_size, (m_data_size - m_meta->write_index) + read_index - 1);
        }
    }

    std::size_t contig_size = m_data_size - m_meta->write_index;
    void* buffer1 = static_cast<uint8_t*>(m_data) + m_meta->write_index;
    std::size_t size1 = max_size > contig_size ? contig_size : max_size;
    void* buffer2 = static_cast<uint8_t*>(m_data);
    std::size_t size2 = max_size > contig_size ? max_size - contig_size : 0;
    return std::make_tuple(buffer1, size1, buffer2, size2);
}

void bus::commit(std::size_t bytes)
{
    m_meta->write_index = (m_meta->write_index + bytes) & m_data_size_mask;
}

std::size_t bus::read_index() const
{
    return m_meta->write_index;
}

std::tuple<const void*, std::size_t, const void*, std::size_t> bus::read_buffer(std::size_t read_index) const
{
    const void* buffer1 = static_cast<const uint8_t*>(m_data) + read_index;
    const void* buffer2 = static_cast<const uint8_t*>(m_data);
    if(read_index <= m_meta->write_index)
        return std::make_tuple(buffer1, m_meta->write_index - read_index, buffer2, 0);
    return std::make_tuple(buffer1, m_data_size - read_index, buffer2, m_meta->write_index);
}

std::size_t bus::consume_read_index(std::size_t read_index, std::size_t bytes) const
{
    return (read_index + bytes) & m_data_size_mask;
}

volatile detail::mandatory_consumer_data* bus::open_mandatory_consumer(uint8_t id)
{
    // make sure id is not zero
    if(id == 0)
        throw zero_id();

    volatile detail::mandatory_consumer_data* empty_consumer(NULL);
    for(unsigned int i = 0; i < m_meta->max_mandatory_consumers; ++i)
    {
        volatile detail::mandatory_consumer_data* consumer = m_meta->mandatory_consumers + i;
        if(id == consumer->id)
            return consumer;
        else if(not empty_consumer and consumer->id == 0)
            empty_consumer = consumer;
    }

    empty_consumer->id = id;
    empty_consumer->read_index = read_index();
    return empty_consumer;
}

void bus::close_mandatory_consumer(const volatile detail::mandatory_consumer_data* data)
{
    volatile detail::mandatory_consumer_data* consumer = m_meta->mandatory_consumers + (data - m_meta->mandatory_consumers);
    consumer->id = 0;
}

const volatile detail::meta_page* bus::meta_page() const
{
    return m_meta;
}

std::string bus::mutex_name(const std::string& shmName)
{
    return shmName + "_mutex";
}

std::string bus::condition_name(const std::string& shmName)
{
    return shmName + "_condition";
}

}
