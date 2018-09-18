#include <shmbus/producer.hpp>
#include <shmbus/consumer.hpp>

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
    shmbus::producer p(shmbus::open, name);
    while(true)
    {
        p.write_some(data.data(), data.size());
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
    shmbus::consumer c(shmbus::open, name);
    while(true)
    {
        const void* data;
        std::size_t dataSize;
        std::tie(data, dataSize) = c.data();
        if(dataSize == 0)
        {
            std::cout << "waiting" << std::endl;
            c.wait(boost::get_system_time() + boost::posix_time::seconds(1));
        }
        else
        {
            std::cout << "rx " << dataSize << " bytes" << std::endl;
            c.consume(dataSize);
        }
    }
}

int main()
{
    shmbus::producer p(shmbus::create, name, 10);

    std::vector<std::thread> ps;

    for(unsigned int i(0); i < 10; ++i)
        ps.emplace_back(producer);

    consumer();
}
