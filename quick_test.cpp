#include "bus_producer.hpp"
#include "bus_consumer.hpp"

#include <boost/thread/thread_time.hpp>

#include <unistd.h>

#include <vector>
#include <iostream>
#include <thread>
#include <chrono>

std::string name("shmbus_test");
std::string data("0123456789");

void producer()
{
    bus_producer p(bus::open(), name);
    while(true)
    {
        p.write(data.data(), data.size());
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

bool numeric_ordered(const char* data, unsigned int size)
{
    char last = 0;
    while(size > 0)
    {
        if(!last)
            last = *data;
        else
        {
            if(*data - last != 1 and last - *data != 9)
                return false;
            last = *data;
        }
        data += 1;
        size -= 1;
    }
    return true;
}

void consumer()
{
    bus_consumer c(bus::open(), name);
    while(true)
    {
        auto cData = c.data();
        if(cData.second == 0)
            c.wait(boost::get_system_time() + boost::posix_time::seconds(1));
        else
        {
            const char* data = static_cast<const char*>(cData.first);
            if(not numeric_ordered(data, cData.second))
                std::cout << (c.read_offset() & c.get_bus().mask()) << " " << std::string(data, cData.second) << std::endl;
            c.consume(cData.second);
        }
    }
}

int main()
{
    bus_producer p(bus::create(), name, 10);

    std::vector<std::thread> ps;

    for(unsigned int i(0); i < 10; ++i)
        ps.emplace_back(producer);

    consumer();
}
