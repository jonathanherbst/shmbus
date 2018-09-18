#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <string>
#include <unordered_set>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>

namespace asio = boost::asio;

class Server
{
    struct Client
    {
        asio::local::stream_protocol::socket socket;
        char buffer[1024*1024];

        Client(asio::io_service& ioService) :
        socket(ioService)
        {}
    };

public:
    Server(const std::string& name) :
    m_endpointA(name + "_a"),
    m_acceptorA(m_ioService, m_endpointA),
    m_endpointB(name + "_b"),
    m_acceptorB(m_ioService, m_endpointB)
    {}

    void operator()()
    {
        try
        {
            Client* client = new Client(m_ioService);
            m_acceptorA.async_accept(client->socket, boost::bind(&Server::acceptorACallback, this, client, asio::placeholders::error));
            client = new Client(m_ioService);
            m_acceptorB.async_accept(client->socket, boost::bind(&Server::acceptorBCallback, this, client, asio::placeholders::error));
            m_ioService.run();
        }
        catch(const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }

    void stop()
    {
        m_ioService.post([](){ throw std::runtime_error("stop"); });
    }

    void acceptorACallback(Client* client, const boost::system::error_code& error)
    {
        if(error)
        {
            delete client;
            std::cout << "server a: " << error << std::endl;
        }
        else
        {
            m_clientsA.insert(client);
            client->socket.async_read_some(asio::mutable_buffers_1(client->buffer, sizeof(client->buffer)),
                    boost::bind(&Server::readACallback, this, client, asio::placeholders::error, asio::placeholders::bytes_transferred));
            client = new Client(m_ioService);
            m_acceptorA.async_accept(client->socket, boost::bind(&Server::acceptorACallback, this, client, asio::placeholders::error));
        }
    }

    void readACallback(Client* client, const boost::system::error_code& error, std::size_t bytesTransfered)
    {
        if(error)
        {
            m_clientsA.erase(client);
            delete client;
            std::cout << "client a: " << error << std::endl;
        }
        else
        {
            for(Client* writeClient : m_clientsB)
            {
                asio::write(writeClient->socket, asio::const_buffers_1(client->buffer, bytesTransfered));
            }
            client->socket.async_read_some(asio::mutable_buffers_1(client->buffer, sizeof(client->buffer)),
                    boost::bind(&Server::readACallback, this, client, asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
    }

    void acceptorBCallback(Client* client, const boost::system::error_code& error)
    {
        if(error)
        {
            delete client;
            std::cout << "server b: " << error << std::endl;
        }
        else
        {
            m_clientsB.insert(client);
            client->socket.async_read_some(asio::mutable_buffers_1(client->buffer, sizeof(client->buffer)),
                    boost::bind(&Server::readBCallback, this, client, asio::placeholders::error, asio::placeholders::bytes_transferred));
            client = new Client(m_ioService);
            m_acceptorB.async_accept(client->socket, boost::bind(&Server::acceptorBCallback, this, client, asio::placeholders::error));
        }
    }

    void readBCallback(Client* client, const boost::system::error_code& error, std::size_t bytesTransfered)
    {
        if(error)
        {
            m_clientsB.erase(client);
            delete client;
            std::cout << "client a: " << error << std::endl;
        }
        else
        {
            for(Client* writeClient : m_clientsA)
            {
                asio::write(writeClient->socket, asio::const_buffers_1(client->buffer, bytesTransfered));
            }
            client->socket.async_read_some(asio::mutable_buffers_1(client->buffer, sizeof(client->buffer)),
                    boost::bind(&Server::readBCallback, this, client, asio::placeholders::error, asio::placeholders::bytes_transferred));
        }
    }

    const asio::local::stream_protocol::endpoint& endpointA() const
    {
        return m_endpointA;
    }

    const asio::local::stream_protocol::endpoint& endpointB() const
    {
        return m_endpointB;
    }

private:
    typedef std::unordered_set<Client*> Clients;

    asio::io_service m_ioService;
    asio::local::stream_protocol::endpoint m_endpointA;
    asio::local::stream_protocol::acceptor m_acceptorA;
    Clients m_clientsA;
    asio::local::stream_protocol::endpoint m_endpointB;
    asio::local::stream_protocol::acceptor m_acceptorB;
    Clients m_clientsB;
};

class Producer
{
public:
    Producer(const asio::local::stream_protocol::endpoint& endpoint) :
    m_endpoint(endpoint),
    m_socket(m_ioService),
    m_run(true)
    {}

    void operator()()
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine generator(seed);
        std::uniform_int_distribution<unsigned int> dist(10, 1000);
        uint64_t v = 0;
        uint64_t buffer[1000];
        m_socket.connect(m_endpoint);
        while(m_run)
        {
            unsigned int count = dist(generator);
            for(unsigned int i(0); i < count; ++i)
                buffer[i] = v++;
            asio::write(m_socket, asio::const_buffers_1(buffer, count * sizeof(v)));
        }
    }

    void stop()
    {
        m_run = false;
    }

private:
    asio::io_service m_ioService;
    asio::local::stream_protocol::endpoint m_endpoint;
    asio::local::stream_protocol::socket m_socket;
    bool m_run;
};

class Consumer
{
public:
    Consumer(const asio::local::stream_protocol::endpoint& endpoint) :
    m_endpoint(endpoint),
    m_socket(m_ioService),
    m_run(true),
    m_byteCount(0)
    {}

    void operator()()
    {
        uint64_t lastV(-1);
        m_socket.connect(m_endpoint);
        uint64_t buffer[1000];

        while(m_run)
        {
            m_byteCount += m_socket.read_some(asio::mutable_buffers_1(buffer, sizeof(lastV) * 1000));
        }
    }

    unsigned int byteCount() const
    {
        return m_byteCount;
    }

private:
    asio::io_service m_ioService;
    asio::local::stream_protocol::endpoint m_endpoint;
    asio::local::stream_protocol::socket m_socket;
    bool m_run;
    unsigned int m_byteCount;
};

int main()
{
    Server server(std::string("/tmp/test"));
    std::thread serverThread([&server](){ server(); });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    Producer producer(server.endpointA());
    Consumer consumer(server.endpointB());
    std::thread consumerThread([&consumer](){consumer();});
    std::thread producerThread([&producer](){producer();});

    std::this_thread::sleep_for(std::chrono::minutes(1));

    std::cout << consumer.byteCount() << std::endl;
    
    producer.stop();
}
