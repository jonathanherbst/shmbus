#include <boost/asio.hpp>

#include <shmbus/producer.hpp>
#include <shmbus/mandatory_consumer.hpp>

#include <chrono>
#include <iostream>

namespace asio = boost::asio;

class SerialPortReader
{
public:
    SerialPortReader(asio::serial_port& port, shmbus::producer& producer) :
    m_port(port),
    m_producer(producer),
    m_bufferPtr(asio::buffer(m_buffer))
    {
        m_port.async_read_some(m_bufferPtr, std::bind(&SerialPortReader::rxCallback, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
    }

private:
    asio::serial_port& m_port;
    shmbus::producer& m_producer;
    std::array<uint8_t, 4096> m_buffer;
    asio::mutable_buffer m_bufferPtr;

    void rxCallback(const boost::system::error_code& error, unsigned int bytesTransferred)
    {
        if(not error)
        {
            unsigned int bytesWritten = m_producer.write_some(asio::buffer_cast<const void*>(m_bufferPtr), bytesTransferred);
            if(bytesWritten < bytesTransferred)
            {
                m_bufferPtr = m_bufferPtr + bytesWritten;
                m_port.get_io_service().post(std::bind(&SerialPortReader::rxCallback, this, error, bytesTransferred - bytesWritten));
            }
            else
            {
                m_bufferPtr = asio::buffer(m_buffer);
                m_port.async_read_some(m_bufferPtr, std::bind(&SerialPortReader::rxCallback, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
            }
        }
    }
};

class SerialPortWriter
{
public:
    SerialPortWriter(asio::serial_port& port, shmbus::mandatory_consumer& consumer) :
    m_port(port),
    m_consumer(consumer),
    m_timer(port.get_io_service())
    {
        transmitOrWait();
    }

private:
    asio::serial_port& m_port;
    shmbus::mandatory_consumer& m_consumer;
    asio::steady_timer m_timer;

    void txCallback(const boost::system::error_code& error, unsigned int bytesTransferred)
    {
        if(not error)
        {
            m_consumer.consume(bytesTransferred);
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
        std::tie(data, dataSize) = m_consumer.data();
        if(dataSize > 0)
            m_port.async_write_some(asio::buffer(data, dataSize), std::bind(&SerialPortWriter::txCallback, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
        else
        {
            m_timer.expires_after(std::chrono::milliseconds(10));
            m_timer.async_wait(std::bind(&SerialPortWriter::timeout, this, asio::placeholders::error));
        }
    }
};

void run_forever(const std::string& busName, const std::string& path, unsigned int baudRate,
    asio::serial_port::flow_control::type flowControl = asio::serial_port::flow_control::none)
{
    asio::io_service ioService;
    asio::serial_port serial(ioService, path);
    serial.set_option(asio::serial_port::baud_rate(baudRate));
    serial.set_option(asio::serial_port::flow_control(flowControl));

    std::string rxBusName = busName + "_rx";
    shmbus::producer rxBus(shmbus::create, rxBusName, 20);
    
    std::string txBusName = busName + "_tx";
    uint8_t id[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    shmbus::mandatory_consumer txBus(shmbus::create, txBusName, 20, id);

    SerialPortReader reader(serial, rxBus);
    SerialPortWriter writer(serial, txBus);
    ioService.run();
}

int main(int argc, char* argv[])
{
    if(argc < 4 or argc > 5)
    {
        std::cout << "Usage: " << std::string(argv[0]) << " <bus_name> <serial_path> <baud_rate> [hwfc]" << std::endl;
        return 1;
    }

    std::string busName(argv[1]);
    std::string serialPath(argv[2]);
    unsigned int baudRate = std::stoul(std::string(argv[2]));
    asio::serial_port::flow_control::type flowControl = asio::serial_port::flow_control::none;
    if(argc > 4 and std::string(argv[4]) == "hwfc")
        flowControl = asio::serial_port::flow_control::hardware;

    run_forever(busName, serialPath, baudRate, flowControl);
    return 0;
}