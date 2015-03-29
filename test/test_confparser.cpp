#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ConfParserModule

#include <cstdio>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "../confparser.hpp"

const char *YAML_TEST_FILE = "conf/test.yaml";
const char *CONF_TEST_FILE = "conf/targetdevice.yaml";

using namespace std;


BOOST_AUTO_TEST_CASE(test_raw_parsing) {
    FILE *fp;
    fp = fopen(YAML_TEST_FILE, "r");
    if(fp == NULL) {
        perror(YAML_TEST_FILE);
        throw runtime_error(string("Cannot open sample file: ") + YAML_TEST_FILE);
    }

    MapElement *res, *tmp, *deep;

    res = dynamic_cast<MapElement*>(raw_conf_parse(fp));

    BOOST_CHECK_EQUAL(res->size(), 3);
    tmp = dynamic_cast<MapElement*>((*res)["connection"]);
    BOOST_CHECK_EQUAL(tmp->size(), 3);
    BOOST_CHECK_EQUAL(*dynamic_cast<ScalarElement*>((*tmp)["host"]), "localhost");
    BOOST_CHECK_EQUAL(*dynamic_cast<ScalarElement*>((*tmp)["port"]), "10023");
    BOOST_CHECK_EQUAL(*dynamic_cast<ScalarElement*>((*tmp)["identity"]), "client_001");

    tmp = dynamic_cast<MapElement*>((*res)["daemon"]);
    BOOST_CHECK_EQUAL(tmp->size(), 3);
    BOOST_CHECK_EQUAL(*dynamic_cast<ScalarElement*>((*tmp)["logfile"]), "/var/log/targetdevice.log");
    BOOST_CHECK_EQUAL(*dynamic_cast<ScalarElement*>((*tmp)["pidfile"]), "/var/pids/targetdevice.pid");
    BOOST_CHECK_EQUAL(*dynamic_cast<ScalarElement*>((*tmp)["serial"]), "/dev/usb/tts/0");

    tmp = dynamic_cast<MapElement*>((*res)["rules"]);
    BOOST_CHECK_EQUAL(tmp->size(), 6);

    deep = dynamic_cast<MapElement*>((*tmp)["is-connected"]);
    BOOST_CHECK_EQUAL(deep->size(), 1);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>((*deep)["return"])), "bit");

    deep = dynamic_cast<MapElement*>((*tmp)["relay-set"]);
    BOOST_CHECK_EQUAL(deep->size(), 2);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>((*deep)["return"])), "bit");
    MapElement &params = *(dynamic_cast<MapElement*>((*deep)["params"]));
    BOOST_CHECK_EQUAL(params.size(), 2);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>(params["port"])), "integer");
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>(params["value"])), "bit");

    deep = dynamic_cast<MapElement*>((*tmp)["line-get"]);
    BOOST_CHECK_EQUAL(deep->size(), 2);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>((*deep)["return"])), "bit");
    MapElement &params1 = *(dynamic_cast<MapElement*>((*deep)["params"]));
    BOOST_CHECK_EQUAL(params1.size(), 1);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>(params1["lineno"])), "integer");

    deep = dynamic_cast<MapElement*>((*tmp)["line-set"]);
    BOOST_CHECK_EQUAL(deep->size(), 2);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>((*deep)["return"])), "bit");
    MapElement &params2 = *(dynamic_cast<MapElement*>((*deep)["params"]));
    BOOST_CHECK_EQUAL(params.size(), 2);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>(params2["lineno"])), "integer");
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>(params2["value"])), "bit");

    deep = dynamic_cast<MapElement*>((*tmp)["adc-get"]);
    BOOST_CHECK_EQUAL(deep->size(), 2);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>((*deep)["return"])), "float");
    MapElement &params3 = *(dynamic_cast<MapElement*>((*deep)["params"]));
    BOOST_CHECK_EQUAL(params1.size(), 1);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>(params3["channel"])), "integer");

    deep = dynamic_cast<MapElement*>((*tmp)["is-connected"]);
    BOOST_CHECK_EQUAL(deep->size(), 1);
    BOOST_CHECK_EQUAL(*(dynamic_cast<ScalarElement*>((*deep)["return"])), "bit");

    delete res;
}


BOOST_AUTO_TEST_CASE(test_conf_parsing) {
    FILE *fp;
    fp = fopen(CONF_TEST_FILE, "r");
    if(fp == NULL) {
        perror(CONF_TEST_FILE);
        throw runtime_error(string("Cannot open sample file: ") + CONF_TEST_FILE);
    }

    MapElement *res, *tmp, *deep;

    res = dynamic_cast<MapElement*>(raw_conf_parse(fp));

    auto_ptr<Config> conf(config_parse(res));

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
    Switcher *pswitcher = dynamic_cast<Switcher*>(conf->devices().at("switcher"));
    BOOST_CHECK(pswitcher != (Switcher*)NULL);
    BOOST_CHECK_EQUAL(pswitcher->relay().driver, "targetdevice");
    BOOST_CHECK_EQUAL(pswitcher->relay().port, 2);
    Thermoswitcher *ptermo = dynamic_cast<Thermoswitcher*>(conf->devices().at("temperature"));
    BOOST_CHECK(ptermo != (Thermoswitcher*)NULL);
    BOOST_CHECK_EQUAL(ptermo->temperature().driver, "targetdevice");
    BOOST_CHECK_EQUAL(ptermo->temperature().port, 1);

    delete res;
}


void parse_test_fnc(FILE *fp) {
    auto_ptr<MapElement> res(dynamic_cast<MapElement*>(raw_conf_parse(fp)));
    auto_ptr<Config> conf(config_parse(res.get()));
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
