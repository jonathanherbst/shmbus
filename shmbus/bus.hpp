#pragma once

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

#include <string>
#include <numeric>

namespace shmbus {

namespace ip = boost::interprocess;

namespace detail {

struct mandatory_consumer_data
{
public:
    uint8_t id[16];
    std::size_t read_index;

    mandatory_consumer_data();

    mandatory_consumer_data(const uint8_t* id, std::size_t read_index);
};

struct meta_page
{
    static const std::size_t max_mandatory_consumers = 8;

    meta_page() :
    write_index(0),
    num_mandatory_consumers(0)
    {}

    std::size_t write_index;
    std::size_t num_mandatory_consumers;
    mandatory_consumer_data mandatory_consumers[max_mandatory_consumers];
};

struct open_ {};
struct create_ {};

}

static const detail::open_ open;
static const detail::create_ create;

class bus
{
public:
    typedef ip::named_mutex mutex_type;
    typedef ip::named_condition condition_type;

    static const ip::offset_t data_offset = sizeof(detail::meta_page);

    static void destroy(const std::string& name);

    bus(detail::create_, const std::string& name, std::size_t sizeExponent);

    bus(detail::open_, const std::string& name);

    std::size_t capacity() const;

    mutex_type& mutex();

    condition_type& condition();

    std::pair<void*, std::size_t> write_buffer();

    void commit(std::size_t bytes);

    std::size_t read_index() const;

    std::pair<const void*, std::size_t> read_buffer(std::size_t read_index) const;

    void consume_read_index(std::size_t& read_index, std::size_t bytes) const;

    detail::mandatory_consumer_data* open_mandatory_consumer(const uint8_t* id);

    void close_mandatory_consumer(const detail::mandatory_consumer_data* data);

private:
    mutex_type m_mutex;
    condition_type m_condition;
    boost::interprocess::shared_memory_object m_mem;
    boost::interprocess::mapped_region m_region;
    volatile detail::meta_page* m_meta;
    void* m_data;
    std::size_t m_data_size;
    std::size_t m_data_size_mask;

    static std::string mutex_name(const std::string& shmName);

    static std::string condition_name(const std::string& shmName);
};

}
