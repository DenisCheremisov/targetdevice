#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ConfParserModule

#include <cstdio>
#include <memory>
#include <cmath>

#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "../confbind.hpp"
#include "../commands.hpp"
#include "initializer.hpp"

using namespace std;

const char *CONFIG_FILE_NAME = "conf/targetdevice.yaml";

ConfigInitializer init(CONFIG_FILE_NAME);


BOOST_AUTO_TEST_CASE(test_driver_creation) {
    Drivers &drivers = *init.drivers;
    Devices &devices = *init.devices;

    Result *r;

    BOOST_CHECK_EQUAL(drivers.serial("targetdevice")->relay_get(2), 0);
    SwitcherOn *swon = new SwitcherOn(devices.device("switcher"));
    r = swon->execute();
    BOOST_CHECK_EQUAL(r->value().c_str(), "1");
    BOOST_CHECK_EQUAL(drivers.serial("targetdevice")->relay_get(2), 1);
    delete swon;

    delete r;

    r = SwitcherOff(devices.device("switcher")).execute();
    BOOST_CHECK_EQUAL(r->value().c_str(), "1");
    BOOST_CHECK_EQUAL(drivers.serial("targetdevice")->relay_get(2), 0);
    delete r;

    r = TemperatureGet(devices.device("temperature")).execute();
    BOOST_CHECK_EQUAL(atof(r->value().c_str()), 0);
    delete r;

    r = TemperatureGet(devices.device("temperature")).execute();
    BOOST_CHECK(fabs(atof(r->value().c_str()) - 1./1023.*5.) < 1e-6);
    delete r;

    r = TemperatureGet(devices.device("temperature")).execute();
    BOOST_CHECK(fabs(atof(r->value().c_str()) - 2./1023.*5.) < 1e-6);
    delete r;

    r = SwitcherOn(devices.device("boiler")).execute();
    BOOST_CHECK_EQUAL(r->value().c_str(), "1");
    delete r;

    r = SwitcherOff(devices.device("boiler")).execute();
    BOOST_CHECK_EQUAL(r->value().c_str(), "1");
    delete r;


    r = TemperatureGet(devices.device("boiler")).execute();
    BOOST_CHECK_EQUAL(atof(r->value().c_str()), 0);
    delete r;

    r = TemperatureGet(devices.device("boiler")).execute();
    BOOST_CHECK(fabs(atof(r->value().c_str()) - 1./1023.*5.) < 1e-6);
    delete r;

    r = TemperatureGet(devices.device("boiler")).execute();
    BOOST_CHECK(fabs(atof(r->value().c_str()) - 2./1023.*5.) < 1e-6);
    delete r;
}
