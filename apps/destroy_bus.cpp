#include <shmbus/bus.hpp>

#include <iostream>

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cout << "Usage: " << std::string(argv[0]) << " <bus_name>" << std::endl;
        return 1;
    }

    std::string busName(argv[1]);

    shmbus::bus::destroy(busName);
}