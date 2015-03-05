#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TargetDeviceDriver

#include <cstdio>

#include <boost/test/unit_test.hpp>

#include "../targetdevice.hpp"

BOOST_AUTO_TEST_CASE(test_no_serial_device) {
    BOOST_REQUIRE_THROW(TargetDeviceDriver a("/some/path/that/does/not/exist"),
                        NoDeviceError);
}

BOOST_AUTO_TEST_CASE(test_correct_serial_device) {
    FILE *fp;
    fp = popen("python3 scripts/virtserial.py", "r");
    if(fp == NULL) {
        throw "Cannot start the virtual serial device";
    }

    char buffer[2048];
    fgets(buffer, 2047, fp);
    fgets(buffer, 2047, fp);
    int i;
    for(i = 0; buffer[i] != '\r' && buffer[i] != '\n'; i++);
    buffer[i] = '\0';

    sleep(1);
    TargetDeviceDriver driver(buffer);

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
