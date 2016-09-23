#pragma once

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

#include <string>
#include <numeric>

namespace detail {

class mandatory_consumer_data
{
public:
    mandatory_consumer_data();

    unsigned int timestamp() const;

    void set_timestamp_now();

    void read_offset(unsigned int offset);

private:
    unsigned int m_timestamp;
    unsigned int m_read_offset;
};

}

class bus
{
public:
    typedef boost::interprocess::named_mutex mutex_type;
    typedef boost::interprocess::named_condition condition_type;

    struct open_ {};
    struct create_ {};

    open_ open;
    create_ create;

    static std::string mutex_name(const std::string& shmName);

    static std::string condition_name(const std::string& shmName);

    static const boost::interprocess::offset_t data_offset;

    bus(create_, const std::string& name, size_t sizeExponent);

    bus(open_, const std::string& name);

    mutex_type& mutex();

    condition_type& condition();

    boost::interprocess::offset_t size() const;

    size_t mask() const;

    void* write_ptr();

    void commit(size_t bytes);

    size_t write_offset() const;

    void* buf_begin() const;

    void* buf_end() const;

private:
    mutex_type m_mutex;
    condition_type m_condition;
    boost::interprocess::shared_memory_object m_mem;
    boost::interprocess::mapped_region m_region;
    size_t m_mask;
    volatile size_t* m_write_offset;
};
