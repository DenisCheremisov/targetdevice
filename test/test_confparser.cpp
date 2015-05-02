#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ConfParserModule

#include <cstdio>
#include <cerrno>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "../confparser.hpp"

const char *YAML_TEST_FILE = "conf/test.yaml";
const char *CONF_TEST_FILE = "conf/test_targetdevice.yaml";

using namespace std;


BOOST_AUTO_TEST_CASE(test_conf_yamlparsing) {
    FILE *fp;
    fp = fopen(CONF_TEST_FILE, "r");
    if(fp == NULL) {
        throw runtime_error(strerror(errno));
    }

    std::auto_ptr<YamlParser> parser(YamlParser::get(fp));
    ConfigStruct config;
    yaml_parse(parser.get(), &config);

    BOOST_CHECK_EQUAL(config.daemon.pidfile.value, "targetdevice.pid");
    BOOST_CHECK_EQUAL(config.daemon.logfile.value, "targetdevice.log");

    BOOST_CHECK_EQUAL(config.connection.host.value, "localhost");
    BOOST_CHECK_EQUAL(config.connection.port.value, 10023);
    BOOST_CHECK_EQUAL(config.connection.identity.value, "client_001");

    BOOST_CHECK_EQUAL(config.drivers.size(), 1);
    BaseStruct *tmp;
    tmp = config.drivers.at("targetdevice");
    BOOST_CHECK(dynamic_cast<SerialDriverStruct*>(tmp) != NULL);
    SerialDriverStruct &driver = *dynamic_cast<SerialDriverStruct*>(tmp);
    BOOST_CHECK_EQUAL(driver.type.value, "serial");
    BOOST_CHECK_EQUAL(driver.path.value, "/dev/pts/19");

    BOOST_CHECK_EQUAL(config.devices.size(), 3);
    tmp = config.devices.at("switcher");
    BOOST_CHECK(tmp->null == false);
    BOOST_CHECK(dynamic_cast<SerialDeviceStruct*>(tmp) != NULL);
    SerialDeviceStruct &switcher = *dynamic_cast<SerialDeviceStruct*>(tmp);
    BOOST_CHECK_EQUAL(switcher.type.value, "switcher");
    BOOST_CHECK_EQUAL(switcher.relay.value, "targetdevice.2");
    BOOST_CHECK(switcher.temperature.null);
    BOOST_CHECK(switcher.factor.null);
    BOOST_CHECK(switcher.shift.null);

    tmp = config.devices.at("temperature");
    BOOST_CHECK(tmp->null == false);
    BOOST_CHECK(dynamic_cast<SerialDeviceStruct*>(tmp) != NULL);
    SerialDeviceStruct &temperature = *dynamic_cast<SerialDeviceStruct*>(tmp);
    BOOST_CHECK_EQUAL(temperature.type.value, "thermalswitcher");
    BOOST_CHECK_EQUAL(temperature.temperature.value, "targetdevice.1");
    BOOST_CHECK_EQUAL(temperature.factor.value, 12.0);
    BOOST_CHECK_EQUAL(temperature.shift.value, 13.0);
    BOOST_CHECK(temperature.relay.null);

    tmp = config.devices.at("boiler");
    BOOST_CHECK(tmp->null == false);
    BOOST_CHECK(dynamic_cast<SerialDeviceStruct*>(tmp) != NULL);
    SerialDeviceStruct &boiler = *dynamic_cast<SerialDeviceStruct*>(tmp);
    BOOST_CHECK_EQUAL(boiler.type.value, "boiler");
    BOOST_CHECK_EQUAL(boiler.relay.value, "targetdevice.1");
    BOOST_CHECK_EQUAL(boiler.temperature.value, "targetdevice.2");
    BOOST_CHECK_EQUAL(boiler.factor.value, 5.0);
    BOOST_CHECK_EQUAL(boiler.shift.value, 6.0);

    UserData userdata;
    config.check(&userdata);

    fclose(fp);
}


BOOST_AUTO_TEST_CASE(test_conf_parsing) {
    FILE *fp;
    fp = fopen(CONF_TEST_FILE, "r");
    BOOST_CHECK(fp != NULL);
    std::auto_ptr<YamlParser> parser(YamlParser::get(fp));
    ConfigStruct rawconf;
    yaml_parse(parser.get(), &rawconf);
    UserData userdata;
    rawconf.check(&userdata);
    auto_ptr<Config> conf(Config::get_from_struct(&rawconf));

    // Check daemon section
    BOOST_CHECK_EQUAL(conf->daemon().logfile, "targetdevice.log");
    BOOST_CHECK_EQUAL(conf->daemon().pidfile, "targetdevice.pid");

    // Check connection section
    BOOST_CHECK_EQUAL(conf->connection().host, "localhost");
    BOOST_CHECK_EQUAL(conf->connection().port, 10023);
    BOOST_CHECK_EQUAL(conf->connection().identity, "client_001");

    // Check drivers
    BOOST_CHECK_EQUAL(conf->drivers().size(), 1);
    SerialDriver *pdriver = dynamic_cast<SerialDriver*>(conf->drivers().at("targetdevice"));
    BOOST_CHECK(pdriver != (SerialDriver*)NULL);
    BOOST_CHECK_EQUAL(pdriver->path(), "/dev/pts/19");

    // Check devices
    BOOST_CHECK_EQUAL(conf->devices().size(), 3);
    Boiler *pboiler = dynamic_cast<Boiler*>(conf->devices().at("boiler"));
    BOOST_CHECK(pboiler != (Boiler*)NULL);
    BOOST_CHECK_EQUAL(pboiler->relay().driver, "targetdevice");
    BOOST_CHECK_EQUAL(pboiler->relay().port, 1);
    BOOST_CHECK_EQUAL(pboiler->temperature().driver, "targetdevice");
    BOOST_CHECK_EQUAL(pboiler->temperature().port, 2);
    BOOST_CHECK_EQUAL(pboiler->factor(), 5);
    BOOST_CHECK_EQUAL(pboiler->shift(), 6);
    Switcher *pswitcher = dynamic_cast<Switcher*>(conf->devices().at("switcher"));
    BOOST_CHECK(pswitcher != (Switcher*)NULL);
    BOOST_CHECK_EQUAL(pswitcher->relay().driver, "targetdevice");
    BOOST_CHECK_EQUAL(pswitcher->relay().port, 2);
    Thermoswitcher *ptermo = dynamic_cast<Thermoswitcher*>(conf->devices().at("temperature"));
    BOOST_CHECK(ptermo != (Thermoswitcher*)NULL);
    BOOST_CHECK_EQUAL(ptermo->temperature().driver, "targetdevice");
    BOOST_CHECK_EQUAL(ptermo->temperature().port, 1);
    BOOST_CHECK_EQUAL(ptermo->factor(), 12);
    BOOST_CHECK_EQUAL(ptermo->shift(), 13);

    fclose(fp);
}


void parse_test_fnc(FILE *fp) {
    std::auto_ptr<YamlParser> parser(YamlParser::get(fp));
    ConfigStruct rawconf;
    try {
        yaml_parse(parser.get(), &rawconf);
    } catch(YamlBaseError e) {
        throw ParserError("handled", e.what());
    }
    UserData userdata;
    rawconf.check(&userdata);
    auto_ptr<Config> conf(Config::get_from_struct(&rawconf));
}


BOOST_AUTO_TEST_CASE(test_wrong_confs) {
    const int N = 9;
    const char* a[9] = {
        "1.yaml", "2.yaml", "3.yaml", "4.yaml", "5.yaml", "6.yaml",
        "dev1.yaml", "dev2.yaml", "dev3.yaml"
    };

    for(int i = 0; i < N; i++) {
        string file_name = string("conf/wrongconfs/") + a[i];
        FILE *fp = fopen(file_name.c_str(), "r");
        // try {
        //     parse_test_fnc(fp);
        // } catch(ParserError e) {
        //     cout << file_name << ": " << e.what() << endl;
        // }
        BOOST_REQUIRE_THROW(parse_test_fnc(fp), ParserError);
        fclose(fp);
    }
}
