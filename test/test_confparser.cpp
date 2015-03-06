#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ConfParserModule

#include <cstdio>

#include <boost/test/unit_test.hpp>

#include "../confparser.hpp"

const char *YAML_TEST_FILE = "conf/test.yaml";

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
    BOOST_REQUIRE_THROW(dynamic_cast<ScalarElement&>(*(*res)["connection"]), bad_cast);
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
}
