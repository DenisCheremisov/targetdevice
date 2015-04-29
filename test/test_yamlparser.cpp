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
        add_optional_field("c", &c);
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


BOOST_AUTO_TEST_CASE(test_rollback) {
    {
        std::auto_ptr<YamlParser> parser(YamlParser::get(
            reinterpret_cast<const unsigned char*>(test_yaml.c_str()),
            test_yaml.size()));
        std::auto_ptr<YamlEvent> ev1(parser->get_event());
        std::auto_ptr<YamlEvent> ev2(parser->get_event());
        BOOST_CHECK(ev1->type != ev2->type);
    }
    {
        std::auto_ptr<YamlParser> parser(YamlParser::get(
            reinterpret_cast<const unsigned char*>(test_yaml.c_str()),
            test_yaml.size()));
        yaml_parser_t *copy = parser->store();
        std::auto_ptr<YamlEvent> ev1(parser->get_event());
        std::auto_ptr<YamlEvent> _1(parser->get_event());
        std::auto_ptr<YamlEvent> _2(parser->get_event());
        std::auto_ptr<YamlEvent> _3(parser->get_event());
        std::auto_ptr<YamlEvent> _4(parser->get_event());
        std::auto_ptr<YamlEvent> _5(parser->get_event());
        parser->restore(copy);
        std::auto_ptr<YamlEvent> ev2(parser->get_event());
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
    BOOST_CHECK(dynamic_cast<IntegerStruct*>(strct.at("a")) != NULL);
    BOOST_CHECK(dynamic_cast<FloatStruct*>(strct.at("b")) != NULL);
    BOOST_CHECK(dynamic_cast<StringStruct*>(strct.at("c")) != NULL);

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
    "  wrongfield: lalala\n";


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
    Config conn;
    BaseStruct *res = yaml_parse(parser.get(), &conn);
    BOOST_CHECK(dynamic_cast<Config*>(res) != NULL);
    BOOST_CHECK_EQUAL(conn.conn.host.value, "localhost");
    BOOST_CHECK_EQUAL(conn.conn.port.value, 10023);
    BOOST_CHECK_EQUAL(conn.conn.identity.value, "client_001");

    std::auto_ptr<YamlParser> wontwork(YamlParser::get(harder_error));
    BOOST_REQUIRE_THROW(
        yaml_parse(wontwork.get(), &conn),
        YamlBaseError);
}
