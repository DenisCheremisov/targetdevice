#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ControllerTestModule

#include <cstdio>
#include <memory>

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "../controller.hpp"

using namespace std;

const char *CONFIG_FILE_NAME = "test_targetdevice.yaml";


class ConfigInitializer {
public:
    Config* conf;
    Devices* devices;
    Drivers *drivers;

    ConfigInitializer() {
        stringstream buf;
        buf << "python3 scripts/make_config.py " << getpid();
        FILE *pp = popen(buf.str().c_str(), "r");
        if(pp == NULL) {
            throw "Cannot start virtual serial device";
        }
        sleep(1);

        FILE *fp = fopen(CONFIG_FILE_NAME, "r");
        if(fp == NULL) {
            perror(CONFIG_FILE_NAME);
            throw runtime_error(string("Cannot open sample file: ") + CONFIG_FILE_NAME);
        }
        auto_ptr<MapElement> res(dynamic_cast<MapElement*>(raw_conf_parse(fp)));
        conf = config_parse(res.get());

        drivers = new Drivers(conf->drivers());
        devices = new Devices(*drivers, conf->devices());
    };

    ~ConfigInitializer() throw() {
        delete conf;
        delete devices;
        delete drivers;
    }
};
ConfigInitializer init;


class ConfigResponseConnection: public BaseConnection {
    int counter;
public:
    ~ConfigResponseConnection() throw() {};
    ConfigResponseConnection(): counter(0) {};

    void set_connection(string host, int port) {};

    bool connected() {
        return true;
    }

    void send(string data) {
        if(counter++ == 0) {
            return;
        }
        BOOST_CHECK_EQUAL(data,
                          "SUCCESS=1\n"
                          "boiler:boiler\n"
                          "switcher:switcher\n"
                          "temperature:temperature\n");
    }

    string receive() {
        return "CONFIG\nGET";
    }
};


class InstrResponseConnection: public BaseConnection {
 public:
    ~InstrResponseConnection() throw() {};
    InstrResponseConnection() {};

    void set_connection(string host, int port) {};

    bool connected() {
        return true;
    }

    void send(string data) {
        if(data.substr(0, 5) == "READY") {
            return;
        }
        string test_sample =
            "SUCCESS=1\n"
            "ID=0xfff0:SUCCESS=1:VALUE=0\n"
            "ID=0xfff1:SUCCESS=1:VALUE=0.00488759\n"
            "ID=0xfff2:SUCCESS=1:VALUE=0\n"
            "ID=0xfff3:SUCCESS=1:VALUE=0.00488759\n";
        BOOST_CHECK_EQUAL(data, test_sample);
    }

    string receive() {
        string value =
            "INSTRUCTION\n"
            "ID=0xfff0:TYPE=VALUE:COMMAND=temperature.temperature\n"
            "ID=0xfff1:TYPE=VALUE:COMMAND=temperature.temperature\n"
            "ID=0xfff2:TYPE=VALUE:COMMAND=boiler.temperature\n"
            "ID=0xfff3:TYPE=VALUE:COMMAND=boiler.temperature\n"
            "ID=1:TYPE=SINGLE:NAME=1:COMMAND=boiler.on:START=replaceit:RESTART=2000\n"
            "ID=2:TYPE=COUPLE:NAME=2:COMMAND=boiler.on:COUPLE=boiler.off:COUPLING-INTERVAL=500:START=replaceit:RESTART=2000\n"
            "ID=3:TYPE=CONDITION:NAME=3:COMMAND=boiler.on:COUPLE=boiler.off:START=replaceit:CONDITION=boiler.temperature.LT_50";

        stringstream buf;
        buf << time(NULL) + 4000;
        boost::replace_all(value, "replaceit", buf.str());

        return value;
    }
};


template <class T>
class TestController: public Controller {
public:
    ~TestController() throw() {};
    TestController(Config *_conf,
        Devices *_devices,
        NamedSchedule *_sched): Controller(
            _conf, _devices, _sched) {};

    BaseConnection *get_connection() {
        return new T;
    }
};


BOOST_AUTO_TEST_CASE(test_config_response_server) {
    auto_ptr<NamedSchedule> sched(new NamedSchedule);
    TestController<ConfigResponseConnection> ctrl(
        init.conf, init.devices, sched.get());
    ctrl.execute();
}


BOOST_AUTO_TEST_CASE(test_instruction_response_server) {
    auto_ptr<NamedSchedule> sched(new NamedSchedule);
    TestController<InstrResponseConnection> ctrl(
        init.conf, init.devices, sched.get());
    ctrl.execute();
    BOOST_CHECK_EQUAL(sched->size(), 3);
}
