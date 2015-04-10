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

using namespace std;

const char *CONFIG_FILE_NAME = "test_targetdevice.yaml";


BOOST_AUTO_TEST_CASE(test_driver_creation) {
    FILE *pp;
    stringstream buf;
    buf << "python3 scripts/make_config.py " << getpid();
    pp = popen(buf.str().c_str(), "r");
    if(pp == NULL) {
        throw "Cannot start the virtual serial device";
    }
    sleep(1);

    FILE *fp = fopen(CONFIG_FILE_NAME, "r");
    if(fp == NULL) {
        perror(CONFIG_FILE_NAME);
        throw runtime_error(string("Cannot open sample file: ") + CONFIG_FILE_NAME);
    }
    auto_ptr<MapElement> res(dynamic_cast<MapElement*>(raw_conf_parse(fp)));
    auto_ptr<Config> conf(config_parse(res.get()));

    Drivers drivers(conf->drivers());
    Devices devices(drivers, conf->devices());

    Result *r;

    r = SwitcherOn(devices.device("switcher")).execute();
    BOOST_CHECK_EQUAL(r->value().c_str(), "1");
    delete r;

    r = SwitcherOff(devices.device("switcher")).execute();
    BOOST_CHECK_EQUAL(r->value().c_str(), "1");
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
