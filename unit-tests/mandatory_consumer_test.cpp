#include <shmbus/producer.hpp>
#include <shmbus/mandatory_consumer.hpp>

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MandatoryConsumer_Test)

BOOST_AUTO_TEST_CASE(MandatoryConsumerPreventsProducer)
{
    shmbus::bus::destroy("test");

    uint8_t data[1023];

    {
        shmbus::producer producer(shmbus::create, "test", 9);
        shmbus::mandatory_consumer consumer(shmbus::open, "test", 1);

        BOOST_CHECK_GT(producer.write_some(data, sizeof(data)), 0ul);
        BOOST_CHECK_EQUAL(producer.write_some(data, sizeof(data)),  0ul);
    }

    shmbus::bus::destroy("test");
}

BOOST_AUTO_TEST_SUITE_END()