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

    shmbus::bus bus(shmbus::open, busName);
    const volatile auto* page = bus.meta_page();

    std::cout << "bus name: " << busName << std::endl;
    std::cout << "capacity: " << bus.capacity() << " bytes" << std::endl;
    std::cout << "write index: " << page->write_index << std::endl;
    for(int i = 0; i < page->max_mandatory_consumers; ++i)
    {
        if(page->mandatory_consumers[i].id != 0)
            std::cout << "mandatory consumer " << page->mandatory_consumers[i].id <<
                " read index " << page->mandatory_consumers[i].read_index << std::endl;
    }

    return 0;
}
