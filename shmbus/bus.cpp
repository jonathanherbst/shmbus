#include "bus.hpp"
#include "errors.hpp"

#include <algorithm>

using namespace boost::interprocess;

namespace shmbus {

namespace detail {

mandatory_consumer_data::mandatory_consumer_data() :
id{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
{}

mandatory_consumer_data::mandatory_consumer_data(const uint8_t* id, std::size_t read_index) :
read_index(read_index)
{
    memcpy(this->id, id, 16);
    //std::copy(this->id, this->id + 16, id);
}

}

void bus::destroy(const std::string& name)
{
    named_mutex::remove(mutex_name(name).c_str());
    named_condition::remove(mutex_name(name).c_str());
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
    m_data_size = m_region.get_size() - sizeof(detail::meta_page);
    m_data_size_mask = m_data_size - 1;
}

bus::bus(detail::open_, const std::string& name):
m_mutex(open_only, mutex_name(name).c_str()),
m_condition(open_only, condition_name(name).c_str()),
m_mem(open_only, name.c_str(), read_write),
m_region(m_mem, read_write),
m_meta(static_cast<volatile detail::meta_page*>(m_region.get_address())),
m_data(static_cast<uint8_t*>(m_region.get_address()) + sizeof(detail::meta_page)),
m_data_size(m_region.get_size() - sizeof(detail::meta_page)),
m_data_size_mask(m_data_size - 1)
{}

bus::mutex_type& bus::mutex()
{
    return m_mutex;
}

bus::condition_type& bus::condition()
{
    return m_condition;
}

std::pair<void*, std::size_t> bus::write_buffer()
{
    // find the max index from the mandatory consumers
    std::size_t max_index = m_data_size;
    for(unsigned int i(0); i < m_meta->num_mandatory_consumers; ++i)
    {
        std::size_t read_index = m_meta->mandatory_consumers[i].read_index;
        if(read_index == 0 and m_meta->write_index > 0)
            max_index = std::min(max_index, m_data_size - 1);
        else if(read_index > m_meta->write_index)
            max_index = std::min(max_index, read_index);
    }

    void* buffer = static_cast<uint8_t*>(m_data) + m_meta->write_index;
    return std::make_pair(buffer, max_index - m_meta->write_index);
}

void bus::commit(std::size_t bytes)
{
    m_meta->write_index = (m_meta->write_index + bytes) & m_data_size_mask;
}

std::size_t bus::read_index() const
{
    return m_meta->write_index;
}

std::pair<const void*, std::size_t> bus::read_buffer(std::size_t read_index) const
{
    const void* buffer = static_cast<const uint8_t*>(m_data) + read_index;
    if(read_index <= m_meta->write_index)
        return std::make_pair(buffer, m_meta->write_index - read_index);
    return std::make_pair(buffer, m_data_size - read_index);
}

void bus::consume_read_index(std::size_t& read_index, std::size_t bytes) const
{
    read_index = (read_index + bytes) & m_data_size_mask;
}

detail::mandatory_consumer_data* bus::open_mandatory_consumer(const uint8_t* id)
{
    // make sure id is not zero
    if(std::all_of(id, id + 16, [](uint8_t b) {return b == 0;}))
        throw zero_id();

    detail::mandatory_consumer_data new_consumer_data(id, read_index());

    detail::mandatory_consumer_data* empty_consumer(NULL);
    for(unsigned int i = 0; i < m_meta->max_mandatory_consumers; ++i)
    {
        volatile detail::mandatory_consumer_data* consumer = m_meta->mandatory_consumers + i;
        if(std::equal(id, id + 16, consumer->id))
            return const_cast<detail::mandatory_consumer_data*>(consumer);
        else if(not empty_consumer and std::all_of(consumer->id, consumer->id + 16, [](uint8_t b) {return b == 0;}))
            empty_consumer = const_cast<detail::mandatory_consumer_data*>(consumer);
    }
    *empty_consumer = new_consumer_data;
    return empty_consumer;
}

void bus::close_mandatory_consumer(const detail::mandatory_consumer_data* data)
{
    volatile detail::mandatory_consumer_data* consumer = m_meta->mandatory_consumers + (data - m_meta->mandatory_consumers);
    std::fill(consumer->id, consumer->id + 16, 0);
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
