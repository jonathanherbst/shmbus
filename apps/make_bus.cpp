#include <shmbus/producer.hpp>

#include <iostream>
#include <cstdint>

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        std::cout << "Usage: " << std::string(argv[0]) << " <bus_name> <bus_size>" << std::endl;
        return 1;
    }

    std::string busName(argv[1]);
    std::size_t busSize(std::stoul(argv[2]));

    shmbus::producer(shmbus::create, busName, busSize);
}