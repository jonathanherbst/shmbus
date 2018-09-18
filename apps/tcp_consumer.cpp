#include <boost/asio.hpp>

#include <shmbus/producer.hpp>
#include <shmbus/consumer.hpp>

#include <memory>
#include <chrono>
#include <iostream>
#include <list>

namespace asio = boost::asio;

class TCPClient : public std::enable_shared_from_this<TCPClient>
{
public:
    TCPClient(asio::io_context& ioContext, shmbus::producer& rxBus) :
    m_socket(ioContext),
    m_timer(ioContext),
    m_rxBus(rxBus)
    {}

    asio::ip::tcp::socket& socket()
    {
        return m_socket;
    }

    void run(const std::string& txBusName)
    {
        m_txBus = std::make_unique<shmbus::consumer>(shmbus::open, txBusName);
        receive();
        transmitOrWait();
    }

private:
    asio::ip::tcp::socket m_socket;
    asio::steady_timer m_timer;
    shmbus::producer& m_rxBus;
    std::unique_ptr<shmbus::consumer> m_txBus;
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
                asio::post(m_socket.get_io_context(), std::bind(&TCPClient::rxCallback, shared_from_this(), error, bytesTransmitted - bytesWritten));
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
            m_txBus->consume(bytesTransmitted);
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
        std::tie(data, dataSize) = m_txBus->data();

        if(dataSize > 0)
        {
            m_socket.async_write_some(asio::buffer(data, dataSize), std::bind(&TCPClient::txCallback, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
        else
        {
            m_timer.expires_after(std::chrono::milliseconds(10));
            m_timer.async_wait(std::bind(&TCPClient::timeout, shared_from_this(), asio::placeholders::error));
        }
    }

    void receive()
    {
        m_bufferPtr = asio::buffer(m_buffer.data(), m_buffer.size());
        m_socket.async_read_some(m_bufferPtr, std::bind(&TCPClient::rxCallback, shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred));
    }
};

class TCPServer
{
public:
    TCPServer(asio::io_context& ioContext, shmbus::producer& rxBus, const std::string& txBusName, uint16_t port, unsigned int maxConnections) :
    m_v4Endpoint(asio::ip::tcp::v4(), port),
    m_v6Endpoint(asio::ip::tcp::v6(), port),
    m_v4Acceptor(ioContext, m_v4Endpoint),
    m_v6Acceptor(ioContext, m_v6Endpoint),
    m_rxBus(rxBus),
    m_txBusName(txBusName),
    m_maxClients(maxConnections)
    {
        m_v4Acceptor.listen(10);
        acceptV4();
        m_v6Acceptor.listen(10);
        acceptV6();
    }

private:
    asio::ip::tcp::endpoint m_v4Endpoint;
    asio::ip::tcp::endpoint m_v6Endpoint;
    asio::ip::tcp::acceptor m_v4Acceptor;
    asio::ip::tcp::acceptor m_v6Acceptor;
    shmbus::producer& m_rxBus;
    std::string m_txBusName;
    std::shared_ptr<TCPClient> m_nextV4Client;
    std::shared_ptr<TCPClient> m_nextV6Client;
    std::list<std::weak_ptr<TCPClient>> m_clients;
    unsigned int m_maxClients;

    void cleanClients()
    {
        m_clients.remove_if([](std::weak_ptr<TCPClient> client){return client.expired();});
    }

    void acceptV4Callback(const boost::system::error_code& error)
    {
        if(not error)
        {
            cleanClients();
            if(m_clients.size() < m_maxClients)
            {
                m_clients.emplace_back(m_nextV4Client);
                m_nextV4Client->run(m_txBusName);
            }
            acceptV4();
        }
    }

    void acceptV6Callback(const boost::system::error_code& error)
    {
        if(not error)
        {
            cleanClients();
            if(m_clients.size() < m_maxClients)
            {
                m_clients.emplace_back(m_nextV6Client);
                m_nextV6Client->run(m_txBusName);
            }
            acceptV6();
        }
    }

    void acceptV4()
    {
        m_nextV4Client.reset(new TCPClient(m_v4Acceptor.get_io_context(), m_rxBus));
        m_v4Acceptor.async_accept(m_nextV4Client->socket(), std::bind(&TCPServer::acceptV4Callback, this, asio::placeholders::error));
    }

    void acceptV6()
    {
        m_nextV6Client.reset(new TCPClient(m_v6Acceptor.get_io_context(), m_rxBus));
        m_v6Acceptor.async_accept(m_nextV6Client->socket(), std::bind(&TCPServer::acceptV6Callback, this, asio::placeholders::error));
    }
};

int main(int argc, char* argv[])
{
    if(argc != 4)
    {
        std::cout << "Usage: " << std::string(argv[0]) << " <bus_name> <port> <max_connections>" << std::endl;
        return 1;
    }

    std::string busName(argv[1]);
    uint16_t port = std::stoul(std::string(argv[2]));
    unsigned int maxConnections = std::stoul(std::string(argv[3]));

    std::string rxBusName = busName + "_rx";
    shmbus::producer rxBus(shmbus::open, rxBusName);

    std::string txBusName = busName + "_tx";

    asio::io_context ioContext;
    TCPServer server(ioContext, rxBus, txBusName, port, maxConnections);

    ioContext.run();
}