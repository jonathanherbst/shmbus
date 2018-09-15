#include <shmbus/producer.hpp>
#include <shmbus/consumer.hpp>

#include <string>
#include <thread>
#include <iostream>
#include <random>

std::string name("shmbus_test");

bool run = true;

void producer()
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<unsigned int> dist(10, 1000);
    shmbus::producer p(shmbus::open, name);
    uint64_t v = 0;
    uint64_t buffer[1000];

    while(run)
    {
        unsigned int count = dist(generator);
        for(unsigned int i(0); i < count; ++i)
            buffer[i] = v++;
        p.write_some(buffer, count * sizeof(v));
    }
}

void consumer(uint64_t& bytesConsumed, unsigned int& badCount)
{
    shmbus::consumer c(shmbus::open, name);
    uint64_t lastV = -1;

    while(run)
    {
        auto cData = c.data();
        if(cData.second > 0)
        {
            for(unsigned int i(0); i < cData.second; i += sizeof(lastV))
            {
                uint64_t data;
                memcpy(&data, static_cast<const uint8_t*>(cData.first) + i, sizeof(data));
                badCount += data != lastV + 1 ? 1 : 0;
                lastV = data;
            }
        }
        bytesConsumed += cData.second;
        c.consume(cData.second);
    }
}

int main()
{
    shmbus::producer p(shmbus::create, name, 20);

    unsigned int badCount = 0;
    uint64_t bytesConsumed = 0;

    std::thread pr(producer);
    std::thread c(std::bind(consumer, std::ref(bytesConsumed), std::ref(badCount)));

    std::this_thread::sleep_for(std::chrono::minutes(1));
    run = false;
    pr.join();
    c.join();

    std::cout << bytesConsumed << " " << badCount << std::endl;
}
