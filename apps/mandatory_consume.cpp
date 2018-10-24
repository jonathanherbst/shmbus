#include <shmbus/mandatory_consumer.hpp>

#include <iostream>

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <bus_name> <consumer_id>" << std::endl;
        return 1;
    }

    std::string busName(argv[1]);
    unsigned int id = std::stoul(std::string(argv[2]));

    shmbus::mandatory_consumer bus(shmbus::open, busName, static_cast<uint8_t>(id));

    while(true)
    {
        bus.wait_for_data(std::chrono::seconds(1));

        const void* data;
        std::size_t dataSize;
        std::tie(data, dataSize) = bus.data();

        if(dataSize > 0)
        {
            std::cout << "got " << dataSize << " bytes" << std::endl;
            bus.consume(dataSize);
        }
    }

    return 0;
}