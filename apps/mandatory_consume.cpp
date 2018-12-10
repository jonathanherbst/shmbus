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

        const void* data1;
        std::size_t dataSize1;
        const void* data2;
        std::size_t dataSize2;
        std::tie(data1, dataSize1, data2, dataSize2) = bus.data();

        if(dataSize1 + dataSize2 > 0)
        {
            std::cout << "got " << dataSize1 + dataSize2 << " bytes" << std::endl;
            bus.consume(dataSize1 + dataSize2);
        }
    }

    return 0;
}