#define BOOST_TEST_IGNORE_NON_ZERO_CHILD_CODE
#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TargetDeviceDriver

#include <cstdio>
#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "../targetdevice.hpp"

BOOST_AUTO_TEST_CASE(test_correct_serial_device) {
    // std::stringstream buf;
    SerialCommunicator *comm = new SerialCommunicator("/dev/pts/3");

    sleep(0.5);
    TargetDeviceDriver driver(comm);

    BOOST_CHECK(driver.connected());

    // Test lines
    BOOST_CHECK_EQUAL(driver.line_get(1), 0);
    BOOST_CHECK_EQUAL(driver.line_get(2), 1);
    BOOST_REQUIRE_THROW(driver.line_get(20), TargetDeviceValidationError);
    BOOST_CHECK_EQUAL(driver.line_get_all(), "010000000000000000");
    BOOST_REQUIRE_THROW(driver.line_set(2, 1), TargetDeviceWronglineError);
    BOOST_REQUIRE_THROW(driver.line_set(20, 5), TargetDeviceValidationError);
    BOOST_REQUIRE_THROW(driver.line_set(2, 5), TargetDeviceValidationError);

    // Test io and related lines
    BOOST_CHECK_EQUAL(driver.io_get(1), 0);
    BOOST_CHECK_EQUAL(driver.io_get_all(), "000000000000000000");
    BOOST_REQUIRE_THROW(driver.io_set(19, 2), TargetDeviceValidationError);
    BOOST_REQUIRE_THROW(driver.io_set(1, 2), TargetDeviceValidationError);
    driver.io_set(1, 1);
    BOOST_CHECK_EQUAL(driver.io_get(1), 1);
    BOOST_CHECK_EQUAL(driver.io_get_all(), "100000000000000000");
    BOOST_REQUIRE_THROW(driver.line_get(1), TargetDeviceWronglineError);
    BOOST_CHECK_EQUAL(driver.line_get_all(), "x10000000000000000");
    driver.line_set(1, 1);
    driver.io_set(1, 0);
    BOOST_CHECK_EQUAL(driver.line_get(1), 1);
    BOOST_CHECK_EQUAL(driver.line_get_all(), "110000000000000000");

    // Test relay
    BOOST_REQUIRE_THROW(driver.relay_set(5, 2), TargetDeviceValidationError);
    BOOST_REQUIRE_THROW(driver.relay_set(5, 1), TargetDeviceValidationError);
    driver.relay_set(2, 1);
    driver.relay_set(2, 0);

    // Test adc
    BOOST_REQUIRE_THROW(driver.adc_get(2222), TargetDeviceValidationError);
    BOOST_CHECK_EQUAL(driver.adc_get(2), 0);
    BOOST_CHECK_EQUAL(driver.adc_get(2), 1);
    BOOST_CHECK_EQUAL(driver.adc_get(2), 2);
    BOOST_CHECK_EQUAL(driver.adc_get(1), 0);
    BOOST_CHECK_EQUAL(driver.adc_get(2), 3);
    BOOST_CHECK_EQUAL(driver.adc_get(1), 1);

    // Test set afr
    BOOST_REQUIRE_THROW(driver.afr_set(33333), TargetDeviceValidationError);
    driver.afr_set(333);
    BOOST_REQUIRE_THROW(driver.afr_set(3333), TargetDeviceValidationError);
    driver.afr_set(222);
}
