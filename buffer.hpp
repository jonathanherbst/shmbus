#include <boost/asio.hpp>

class double_buffer
{
public:
    typename boost::asio::mutable_buffer value_type;
    typename const value_type* const_iterator;

    double_buffer(void* b1, size_t b1Size, void* b2, size_t b2Size) :
    m_buffers({boost::asio::mutable_buffer(b1, b1Size), boost::asio::mutable_buffer(b2, b2Size)})
    {}


    const_iterator begin() const { return m_buffers; }
    const_iterator end() const { return m_buffers + 2; }

private:
    boost::asio::mutable_buffer m_buffers[3];
};
