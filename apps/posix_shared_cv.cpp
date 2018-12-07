#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <unistd.h>
#include <pthread.h>

#include <chrono>
#include <thread>
#include <iostream>

namespace ip = boost::interprocess;

struct mutexcv
{
    pthread_mutex_t mutex;
    sem_t semaphore;
    unsigned int sharedCount;
    pthread_cond_t cond;
};

void mutexcvSharedLock(mutexcv* mut)
{
    pthread_mutex_lock(&mut->mutex);
    if(sharedCount > 0)
        sharedCount += 1;
    else
    {
        sem_wait(&mut->semaphore);
        sharedCount += 1;
    }
    pthread_mutex_unlock(&mut->mutex);
}


void mutexcvSharedUnLock(mutexcv* mut)
{
    
    pthread_mutex_lock(&mut->mutex);
    sharedCount -= 1;
    if(sharedCount == 0)
        sem_post(&mut->semaphore);
    pthread_mutex_unlock(&mut->mutex);
}

void mutexcvLock(mutexcv* mut)
{
    sem_wait(&mut->semaphore);
}

void mutexcvUnlock(mutexcv* mut)
{
    sem_post(&mut->semaphore);
}

void setupMutexCV()
{
    ip::shared_memory_object mem(ip::open_or_create, "test_condition", ip::read_write);
    ip::offset_t memorySize;
    if(not mem.get_size(memorySize) or memorySize == 0)
        mem.truncate(sizeof(mutexcv));
    ip::mapped_region mappedMem(mem, ip::read_write);
    mutexcv* data = static_cast<mutexcv*>(mappedMem.get_address());
    data->sharedCount = 0;

    pthread_mutex_destroy(&data->mutex);
    pthread_cond_destroy(&data->cond);
    sem_destroy(&data->semaphore);

    int err;

    pthread_mutexattr_t mutexAttr;
    err = pthread_mutexattr_init(&mutexAttr);
    if(err)
        std::cout << "pthread_mutexattr_init: " << err << std::endl;
    err = pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    if(err)
        std::cout << "pthread_mutexattr_setpshared: " << err << std::endl;
    err = pthread_mutex_init(&data->mutex, &mutexAttr);
    if(err)
        std::cout << "pthread_mutex_init: " << err << std::endl;
    err = pthread_mutexattr_destroy(&mutexAttr);
    if(err)
        std::cout << "pthread_mutexattr_destroy: " << err << std::endl;

    pthread_condattr_t condAttr;
    err = pthread_condattr_init(&condAttr);
    if(err)
        std::cout << "pthread_condattr_init: " << err << std::endl;
    err = pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    if(err)
        std::cout << "pthread_condattr_setpshared: " << err << std::endl;
    err = pthread_cond_init(&data->cond, &condAttr);
    if(err)
        std::cout << "pthread_cond_init: " << err << std::endl;
    err = pthread_condattr_destroy(&condAttr);
    if(err)
        std::cout << "pthread_condattr_destroy: " << err << std::endl;

    sem_init(&data->semaphore, PTHREAD_PROCESS_SHARED, 1);
}

int main()
{
    setupMutexCV();

    pid_t pid = fork();
    if(pid == 0)
    { // parent process
        ip::shared_memory_object mem(ip::open_only, "test_condition", ip::read_write);
        ip::mapped_region mappedMem(mem, ip::read_write);
        mutexcv* data = static_cast<mutexcv*>(mappedMem.get_address());
        int err;

        while(true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "signaling children" << std::endl;
            err = pthread_cond_broadcast(&data->cond);
            if(err != 0)
                std::cout << "broadcast error: " << err << std::endl;
        }
    }
    else if(pid > 0)
    { // child process
        ip::shared_memory_object mem(ip::open_only, "test_condition", ip::read_write);
        ip::mapped_region mappedMem(mem, ip::read_write);
        mutexcv* data = static_cast<mutexcv*>(mappedMem.get_address());
        int err;
        
        while(true)
        {
            err = pthread_mutex_lock(&data->mutex);
            if(err)
                std::cout << "unlock error: " << err << std::endl;

            struct timespec waitTime;
            clock_gettime(CLOCK_REALTIME, &waitTime);
            waitTime.tv_sec += 10;

            err = pthread_cond_timedwait(&data->cond, &data->mutex, &waitTime);
            if(err == 0)
                std::cout << "got signal from parent" << std::endl;
            else
                std::cout << "timedwait error: " << err << std::endl;
            err = pthread_mutex_unlock(&data->mutex);
            if(err)
                std::cout << "unlock error: " << err << std::endl;
        }
    }
    else
    {
        std::cout << "failed to fork: " << pid << std::endl;
        return 1;
    }

    return 0;
}