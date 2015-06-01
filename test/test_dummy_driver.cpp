#define BOOST_TEST_IGNORE_NON_ZERO_CHILD_CODE
#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DummyDriver

#include <boost/test/unit_test.hpp>
#include "drivers.hpp"


BOOST_AUTO_TEST_CASE(test_dummy_driver_backend) {
    TestSerialCommunicator d;
    BOOST_CHECK_EQUAL(d.is_connected(), "#OK");

    // Line read/write correctness check
    BOOST_CHECK_EQUAL(d.ioget_all(), "#IO,000000000000000000");
    BOOST_CHECK_EQUAL(d.ioget(5), "#IO,0");
    BOOST_CHECK_EQUAL(d.write_line(5, 1), "#WR,WRONGLINE");
    BOOST_CHECK_EQUAL(d.read_all_lines(), "#RD,010000000000000000");
    BOOST_CHECK_EQUAL(d.read_line(5), "#RD,5,0");
    BOOST_CHECK_EQUAL(d.ioset(5, DIR_IN), "#IO,SET,OK");
    BOOST_CHECK_EQUAL(d.ioget_all(), "#IO,000010000000000000");
    BOOST_CHECK_EQUAL(d.ioget(5), "#IO,1");
    BOOST_CHECK_EQUAL(d.write_line(5, 1), "#WR,OK");
    BOOST_CHECK_EQUAL(d.read_line(5), "#RD,WRONGLINE");
    BOOST_CHECK_EQUAL(d.read_all_lines(), "#RD,0100x0000000000000");
    BOOST_CHECK_EQUAL(d.ioset(5, DIR_OUT), "#IO,SET,OK");
    BOOST_CHECK_EQUAL(d.write_line(5, 1), "#WR,WRONGLINE");
    BOOST_CHECK_EQUAL(d.read_line(5), "#RD,5,1");
    BOOST_CHECK_EQUAL(d.read_all_lines(), "#RD,010010000000000000");

    // Adc read check
    BOOST_CHECK_EQUAL(d.get_adc(1), "#ADC,1,0000");
    BOOST_CHECK_EQUAL(d.get_adc(1), "#ADC,1,0001");
    BOOST_CHECK_EQUAL(d.get_adc(1), "#ADC,1,0002");

    // Relay read/write check
    BOOST_CHECK_EQUAL(d.read_relay(2), "#RID,2,0");
    BOOST_CHECK_EQUAL(d.write_relay(2, 1), "#REL,OK");
    BOOST_CHECK_EQUAL(d.read_relay(2), "#RID,2,1");
}
