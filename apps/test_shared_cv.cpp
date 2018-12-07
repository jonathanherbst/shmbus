#include <boost/interprocess/sync/named_condition_any.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/thread/thread_time.hpp>

#include <unistd.h>

#include <chrono>
#include <thread>
#include <iostream>

namespace ip = boost::interprocess;

int main()
{
    typedef ip::named_mutex mutex_type;
    //typedef ip::named_condition condition_type;
    typedef ip::named_condition_any condition_type;

    mutex_type::remove("test_mutex");
    condition_type::remove("test_condition");

    pid_t pid = fork();
    if(pid == 0)
    { // parent process
        mutex_type mutex(ip::open_or_create, "test_mutex");
        condition_type condition(ip::open_or_create, "test_condition");

        while(true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "signaling children" << std::endl;
            condition.notify_one();
        }
    }
    else if(pid > 0)
    { // child process
        mutex_type mutex(ip::open_or_create, "test_mutex");
        condition_type condition(ip::open_or_create, "test_condition");
        
        while(true)
        {
            boost::posix_time::ptime wait_time(boost::get_system_time() +
                boost::posix_time::seconds(10));
            ip::scoped_lock<ip::named_mutex> lock(mutex);
            if(condition.timed_wait(lock, wait_time))
                std::cout << "got signal from parent" << std::endl;
            else
                std::cout << "timeout" << std::endl;
        }
    }
    else
    {
        std::cout << "failed to fork: " << pid << std::endl;
        return 1;
    }

    return 0;
}