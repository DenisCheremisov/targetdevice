#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ConfParserModule

#include <cstdio>
#include <memory>

#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "../confbind.hpp"

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
    drivers.serial("targetdevice");

    Devices devices(drivers, conf->devices());
    BOOST_CHECK_EQUAL(devices.device("switcher")->type, DEVICE_SWITCHER);
    BOOST_CHECK_EQUAL(devices.device("temperature")->type, DEVICE_THERMOSWITCHER);
    BOOST_CHECK_EQUAL(devices.device("boiler")->type, DEVICE_BOILER);
}
