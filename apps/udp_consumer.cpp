#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <shmbus/producer.hpp>
#include <shmbus/consumer.hpp>

#include <chrono>
#include <iostream>

namespace asio = boost::asio;

class UDPServer
{
public:
    UDPServer(asio::io_context& ioContext, shmbus::producer& rxBus, shmbus::consumer& txBus, const std::string ipAddress, uint16_t port) :
    m_endpoint(asio::ip::make_address(ipAddress), port),
    m_socket(ioContext, m_endpoint),
    m_timer(ioContext),
    m_rxBus(rxBus),
    m_txBus(txBus)
    {
        receive();
        transmitOrWait();
    }

    const asio::ip::udp::endpoint& endpoint() const
    {
        return m_endpoint;
    }

private:
    asio::ip::udp::endpoint m_endpoint;
    asio::ip::udp::socket m_socket;
    asio::steady_timer m_timer;
    shmbus::producer& m_rxBus;
    shmbus::consumer& m_txBus;
    std::array<uint8_t, 4096> m_buffer;
    asio::mutable_buffer m_bufferPtr;

    void rxCallback(const boost::system::error_code& error, std::size_t bytesTransferred)
    {
        if(not error)
        {
            std::size_t bytesWritten = m_rxBus.write_some(asio::buffer_cast<const void*>(m_bufferPtr), bytesTransferred);
            if(bytesWritten < bytesTransferred)
            {
                m_bufferPtr = m_bufferPtr + bytesWritten;
                asio::post(m_socket.get_io_context(), boost::bind(&UDPServer::rxCallback, this, error, bytesTransferred - bytesWritten));
            }
            else
            {
                receive();
            }
        }
    }

    void txCallback(const boost::system::error_code& error, unsigned int bytesTransferred)
    {
        if(not error)
        {
            m_txBus.consume(bytesTransferred);
            transmitOrWait();
        }
    }

    void timeoutCallback(const boost::system::error_code& error)
    {
        if(not error)
        {
            transmitOrWait();
        }
    }

    void transmitOrWait()
    {
        const void* data;
        std::size_t dataSize;
        std::tie(data, dataSize) = m_txBus.data();

        if(dataSize > 0)
        {
            m_socket.async_send(asio::buffer(data, dataSize), boost::bind(&UDPServer::txCallback, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        else
        {
            m_timer.expires_after(std::chrono::milliseconds(10));
            m_timer.async_wait(boost::bind(&UDPServer::timeoutCallback, this, asio::placeholders::error));
        }
    }

    void receive()
    {
        m_bufferPtr = asio::buffer(m_buffer.data(), m_buffer.size());
        m_socket.async_receive(m_bufferPtr, boost::bind(&UDPServer::rxCallback, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
    }
};

int main(int argc, char* argv[])
{
    if(argc != 4)
    {
        std::cout << "Usage: " << std::string(argv[0]) << " <bus_name> <ip_address> <port>" << std::endl;
        return 1;
    }

    std::string busName(argv[1]);
    std::string ipAddress(argv[2]);
    uint16_t port = std::stoul(std::string(argv[3]));

    std::string rxBusName = busName + "_rx";
    shmbus::producer rxBus(shmbus::open, rxBusName);

    std::string txBusName = busName + "_tx";
    shmbus::consumer txBus(shmbus::open, txBusName);

    asio::io_context ioContext;
    UDPServer server(ioContext, rxBus, txBus, ipAddress, port);

    if(server.endpoint().address().is_multicast())
    {
        asio::ip::multicast::join_group(server.endpoint().address());
    }

    ioContext.run();
}