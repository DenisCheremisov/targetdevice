#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE YamlparserCpp

#include <cstdio>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "../yamlparser.hpp"


std::string test_yaml =
    "a: 1\n"
    "b: 2.0\n"
    "c: hello";


class Struct: public ShellStruct {
public:
    IntegerStruct a;
    FloatStruct b;
    StringStruct c;

    Struct() {
        add_required_field("a", &a);
        add_required_field("b", &b);
        add_required_field("c", &c);
    }
};


BOOST_AUTO_TEST_CASE(test_simple_read) {
    std::auto_ptr<YamlParser> parser(
        YamlParser::get(
            reinterpret_cast<const unsigned char*>(test_yaml.c_str()),
            test_yaml.size()));
    Struct strct;
    BaseStruct *res = yaml_parse(parser.get(), &strct);
    BOOST_CHECK(dynamic_cast<Struct*>(res) != NULL);
    BOOST_CHECK_EQUAL(strct.a.value, 1);
    BOOST_CHECK_EQUAL(strct.b.value, 2.0);
    BOOST_CHECK_EQUAL(strct.c.value, "hello");
}


class OptionalStruct: public Struct {
public:
    StringStruct d;

    OptionalStruct() {
        add_optional_field("d", &d);
    }
};


BOOST_AUTO_TEST_CASE(test_simple_optional) {
    std::auto_ptr<YamlParser> parser(
        YamlParser::get(test_yaml + "\nd: world"));
    OptionalStruct strct;
    BaseStruct *res = yaml_parse(parser.get(), &strct);
    BOOST_CHECK(dynamic_cast<OptionalStruct*>(res) != NULL);
    BOOST_CHECK_EQUAL(strct.a.value, 1);
    BOOST_CHECK_EQUAL(strct.b.value, 2.0);
    BOOST_CHECK_EQUAL(strct.c.value, "hello");
    BOOST_CHECK(strct.d.null == false);
    BOOST_CHECK_EQUAL(strct.d.value, "world");

    std::auto_ptr<YamlParser> parser2(
        YamlParser::get(test_yaml));
    OptionalStruct strct2;
    res = yaml_parse(parser2.get(), &strct2);
    BOOST_CHECK(dynamic_cast<OptionalStruct*>(res) != NULL);
    BOOST_CHECK_EQUAL(strct2.a.value, 1);
    BOOST_CHECK_EQUAL(strct2.b.value, 2.0);
    BOOST_CHECK_EQUAL(strct2.c.value, "hello");
    BOOST_CHECK(strct2.d.null);
}


BOOST_AUTO_TEST_CASE(test_rollback) {
    {
        std::auto_ptr<YamlParser> parser(YamlParser::get(
            reinterpret_cast<const unsigned char*>(test_yaml.c_str()),
            test_yaml.size()));
        YamlEvent *ev1 = parser->get_event();
        YamlEvent *ev2 = parser->get_event();
        BOOST_CHECK(ev1->type != ev2->type);
    }
    {
        std::auto_ptr<YamlParser> parser(YamlParser::get(test_yaml));
        YamlParser prsr(*parser);
        YamlEvent *ev1 = prsr.get_event();
        prsr.get_event();
        prsr.get_event();
        prsr.get_event();
        prsr.get_event();
        prsr.get_event();
        YamlEvent *ev2 = parser->get_event();
        BOOST_CHECK_EQUAL(ev1->type, ev2->type);
    }
}


typedef MappingStruct<ChoiceStruct<IntegerStruct,
                                   ChoiceStruct<
                                       FloatStruct, StringStruct> > > tmptype;
BOOST_AUTO_TEST_CASE(test_variative) {
    std::auto_ptr<YamlParser> parser(
        YamlParser::get(
            reinterpret_cast<const unsigned char*>(test_yaml.c_str()),
            test_yaml.size()));
    ScalarElement a;
    tmptype strct;
    BaseStruct *res = yaml_parse(parser.get(), &strct);
    BOOST_CHECK(dynamic_cast<tmptype*>(res) != NULL);
    BOOST_CHECK_EQUAL(strct.size(), 3);
    BOOST_CHECK_EQUAL(dynamic_cast<IntegerStruct*>(strct.at("a"))->value, 1);
    BOOST_CHECK_EQUAL(dynamic_cast<FloatStruct*>(strct.at("b"))->value, 2.0);
    BOOST_CHECK_EQUAL(dynamic_cast<StringStruct*>(strct.at("c"))->value,
                      "hello");
}


std::string harder =
    "connection:\n"
    "  host: localhost\n"
    "  port: 10023\n"
    "  identity: client_001";


std::string harder_error = harder +
    "\n  wrongfield: lalala\n";


class Connection: public ShellStruct {
public:
    StringStruct host;
    IntegerStruct port;
    StringStruct identity;

    Connection() {
        add_required_field("host", &host);
        add_required_field("port", &port);
        add_required_field("identity", &identity);
    }
};


class Config: public ShellStruct {
public:
    Connection conn;

    Config() {
        add_required_field("connection", &conn);
    }
};


BOOST_AUTO_TEST_CASE(test_real_connector) {
    std::auto_ptr<YamlParser> parser(YamlParser::get(harder));
    Config conf;
    BaseStruct *res = yaml_parse(parser.get(), &conf);
    BOOST_CHECK(dynamic_cast<Config*>(res) != NULL);
    BOOST_CHECK_EQUAL(conf.conn.host.value, "localhost");
    BOOST_CHECK_EQUAL(conf.conn.port.value, 10023);
    BOOST_CHECK_EQUAL(conf.conn.identity.value, "client_001");

    std::auto_ptr<YamlParser> wontwork(YamlParser::get(harder_error));
    BOOST_REQUIRE_THROW(
        yaml_parse(wontwork.get(), &conf),
        YamlBaseError);
}


std::string evenharder =
    harder +
    "\n"
    "devices:\n"
    "  boiler:\n"
    "    type: boiler\n"
    "    relay: targetdevice.1\n"
    "    temperature: targetdevice.2\n"
    "    factor: 1.5\n"
    "    shift: 0\n\n"
    "  switcher:\n"
    "    type: switcher\n"
    "    relay: targetdevice.1\n";
std::string evenharder_error = evenharder +
    "    junk: waste";


class Boiler: public ShellStruct {
public:
    StringStruct type;
    StringStruct relay;
    StringStruct temperature;
    FloatStruct factor;
    FloatStruct shift;

    Boiler() {
        add_required_field("type", &type);
        add_required_field("relay", &relay);
        add_required_field("temperature", &temperature);
        add_required_field("factor", &factor);
        add_required_field("shift", &shift);
    }
};


class Switcher: public ShellStruct {
public:
    StringStruct type;
    StringStruct relay;

    Switcher() {
        add_required_field("type", &type);
        add_required_field("relay", &relay);
    }
};


typedef MappingStruct<ChoiceStruct<Boiler, Switcher> > VariativeStruct;
class LargerConfig: public ShellStruct {
public:
    Connection conn;
    VariativeStruct devices;

    LargerConfig() {
        add_required_field("connection", &conn);
        add_required_field("devices", &devices);
    }
};


BOOST_AUTO_TEST_CASE(test_harder_config) {
    std::auto_ptr<YamlParser> parser(YamlParser::get(evenharder));
    LargerConfig conf;
    BaseStruct *res = yaml_parse(parser.get(), &conf);
    BOOST_CHECK(dynamic_cast<LargerConfig*>(res) != NULL);
    BOOST_CHECK_EQUAL(conf.conn.host.value, "localhost");
    BOOST_CHECK_EQUAL(conf.conn.port.value, 10023);
    BOOST_CHECK_EQUAL(conf.conn.identity.value, "client_001");

    Boiler *blr = dynamic_cast<Boiler*>(conf.devices.at("boiler"));
    BOOST_CHECK(blr != NULL);
    BOOST_CHECK_EQUAL(blr->type.value, "boiler");
    BOOST_CHECK_EQUAL(blr->relay.value, "targetdevice.1");
    BOOST_CHECK_EQUAL(blr->temperature.value, "targetdevice.2");
    BOOST_CHECK_EQUAL(blr->factor.value, 1.5);
    BOOST_CHECK_EQUAL(blr->shift.value, 0);

    Switcher *swt = dynamic_cast<Switcher*>(conf.devices.at("switcher"));
    BOOST_CHECK(swt != NULL);
    BOOST_CHECK_EQUAL(swt->type.value, "switcher");
    BOOST_CHECK_EQUAL(swt->relay.value, "targetdevice.1");
}


BOOST_AUTO_TEST_CASE(test_harder_config_error) {
    std::auto_ptr<YamlParser> parser(YamlParser::get(evenharder_error));
    LargerConfig conf;
    BOOST_REQUIRE_THROW(yaml_parse(parser.get(), &conf), YamlBaseError);
}
