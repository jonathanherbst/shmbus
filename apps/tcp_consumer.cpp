#include <boost/asio.hpp>

#include <shmbus/producer.hpp>
#include <shmbus/consumer.hpp>

#include <chrono>
#include <iostream>

namespace asio = boost::asio;

class TCPClient
{
public:
    TCPClient(asio::io_service& ioService, shmbus::producer& rxBus, shmbus::consumer& txBus) :
    m_socket(ioService),
    m_timer(ioService),
    m_rxBus(rxBus),
    m_txBus(txBus),
    m_bufferPtr(asio::buffer(m_buffer.data(), m_buffer.size()))
    {}

    asio::ip::tcp::socket& socket()
    {
        return m_socket;
    }

    void run()
    {
        receive();
        transmitOrWait();
    }

private:
    asio::ip::tcp::socket m_socket;
    asio::steady_timer m_timer;
    shmbus::producer& m_rxBus;
    shmbus::consumer& m_txBus;
    std::array<uint8_t, 4096> m_buffer;
    asio::mutable_buffer m_bufferPtr;

    void rxCallback(const boost::system::error_code& error, unsigned int bytesTransmitted)
    {
        if(not error)
        {
            unsigned int bytesWritten = m_rxBus.write_some(asio::buffer_cast<const void*>(m_bufferPtr), bytesTransmitted);
            if(bytesWritten < bytesTransmitted)
            {
                m_bufferPtr = m_bufferPtr + bytesWritten;
                m_socket.get_io_service().post(std::bind(&TCPClient::rxCallback, this, error, bytesTransmitted - bytesWritten));
            }
            else
            {
                receive();
            }
        }
    }

    void txCallback(const boost::system::error_code& error, unsigned int bytesTransmitted)
    {
        if(not error)
        {
            m_txBus.consume(bytesTransmitted);
            transmitOrWait();
        }
    }

    void timeout(const boost::system::error_code& error)
    {
        if(not error)
        {
            transmitOrWait();
        }
    }

    void transmitOrWait()
    {
        const void* data;
        unsigned int dataSize;
        std::tie(data, dataSize) = m_txBus.data();

        if(dataSize > 0)
        {
            m_socket.async_write_some(asio::buffer(data, dataSize), std::bind(&TCPClient::txCallback, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        else
        {
            m_timer.expires_after(std::chrono::milliseconds(10));
            m_timer.async_wait(std::bind(&TCPClient::timeout, this, asio::placeholders::error));
        }
    }

    void receive()
    {
        m_bufferPtr = asio::buffer(m_buffer.data(), m_buffer.size());
        m_socket.async_read_some(m_bufferPtr, std::bind(&TCPClient::rxCallback, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
    }
};

class TCPServer
{
public:
    TCPServer(asio::io_service& ioService, shmbus::producer& rxBus, shmbus::consumer& txBus, uint16_t port, unsigned int maxConnections);

private:
    asio::ip::tcp::acceptor m_acceptor;
    shmbus::producer& rxBus;
    shmbus::consumer& txBus;
    std::list<std::shared_ptr<TCPClient>> m_connections;
};