#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ConfParserModule

#include <cstdio>
#include <memory>

#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "../model.hpp"

using namespace std;

const char *CONFIG_FILE_NAME = "test_targetdevice.yaml";


class ConfigInitializer {
public:
    auto_ptr<Config> conf;
    auto_ptr<Devices> devices;

    ConfigInitializer() {
        FILE *pp;
        stringstream buf;
        buf << "python3 scripts/make_config.py " << getpid();
        pp = popen(buf.str().c_str(), "r");
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
        conf = auto_ptr<Config>(config_parse(res.get()));

        Drivers drivers(conf->drivers());
        devices = auto_ptr<Devices>(new Devices(drivers, conf->devices()));
    };
};
ConfigInitializer init;


BOOST_AUTO_TEST_CASE(test_config_get) {
    ConfigInfoModel config_info;
    model_call_params_t params;
    params.config = init.conf.get();
    params.devices = init.devices.get();
    params.request_data = "GET";

    BOOST_CHECK_EQUAL(config_info.execute(params),
                      "boiler:boiler\n"
                      "switcher:switcher\n"
                      "temperature:temperature\n");
}


BOOST_AUTO_TEST_CASE(test_deconstruct) {
    s_map ref;
    BOOST_REQUIRE_THROW(BaseInstructionLine::deconstruct("dddd", ref),
                        InteruptionHandling);
    BOOST_REQUIRE_THROW(BaseInstructionLine::deconstruct("ID=1:COMMAND", ref),
                        InteruptionHandling);
    BOOST_REQUIRE_NO_THROW(BaseInstructionLine::deconstruct("ID=1:TEST=12:",
                                                            ref));
    BOOST_CHECK_EQUAL(ref["ID"], "1");
    BOOST_CHECK_EQUAL(ref["TEST"], "12");
}


BOOST_AUTO_TEST_CASE(test_value_instruction) {
    s_map ref;
    BaseInstructionLine::deconstruct("ID=1:COMMAND=boiler.on", ref);
    ValueInstructionLine instr(ref);
    BOOST_CHECK_EQUAL(instr.id, "1");
    BOOST_CHECK_EQUAL(instr.command, "boiler.on");

    s_map nref;
    BaseInstructionLine::deconstruct("ID=1", nref);
    BOOST_REQUIRE_THROW(
                  auto_ptr<BaseInstructionLine>(new ValueInstructionLine(nref)),
                  InteruptionHandling);

    s_map nnref;
    BaseInstructionLine::deconstruct("COMMAND=nova", nnref);
    BOOST_REQUIRE_THROW(
                 auto_ptr<BaseInstructionLine>(new ValueInstructionLine(nnref)),
                 InteruptionHandling);
}


BOOST_AUTO_TEST_CASE(test_single_instruction) {
    s_map ref;
    BaseInstructionLine::deconstruct(
                           "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12",
                           ref);

    {
        SingleInstructionLine instr(ref);
        BOOST_CHECK_EQUAL(instr.id, "1");
        BOOST_CHECK_EQUAL(instr.name, "boiler-on");
        BOOST_CHECK_EQUAL(instr.command, "boiler.on");
        BOOST_CHECK_EQUAL(instr.start, 12);
        BOOST_CHECK_EQUAL(instr.stop, -1);
        BOOST_CHECK_EQUAL(instr.restart, -1);
    }

    ref.clear();
    BaseInstructionLine::deconstruct(
             "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=13",
             ref);

    {
        SingleInstructionLine instr(ref);
        BOOST_CHECK_EQUAL(instr.id, "1");
        BOOST_CHECK_EQUAL(instr.name, "boiler-on");
        BOOST_CHECK_EQUAL(instr.command, "boiler.on");
        BOOST_CHECK_EQUAL(instr.start, 12);
        BOOST_CHECK_EQUAL(instr.stop, 13);
        BOOST_CHECK_EQUAL(instr.restart, -1);
    }

    ref.clear();
    BaseInstructionLine::deconstruct(
         "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=13:RESTART=50",
         ref);

    {
        SingleInstructionLine instr(ref);
        BOOST_CHECK_EQUAL(instr.id, "1");
        BOOST_CHECK_EQUAL(instr.name, "boiler-on");
        BOOST_CHECK_EQUAL(instr.command, "boiler.on");
        BOOST_CHECK_EQUAL(instr.start, 12);
        BOOST_CHECK_EQUAL(instr.stop, 13);
        BOOST_CHECK_EQUAL(instr.restart, 50);
    }

    const int LENGTH = 8;
    const char *wrong_instrs[LENGTH] = {
        "ID=1:START=12",
        "NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=16:RESTART=50",
        "ID=1:COMMAND=boiler.on:START=12:STOP=16:RESTART=50",
        "ID=1:NAME=boiler-on:START=12:STOP=13:RESTART=50",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:STOP=13:RESTART=50",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=A:STOP=13:RESTART=50",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=A:RESTART=50",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=16:RESTART=a"
    };

    for(int i = 0; i < LENGTH; i++) {
        ref.clear();
        BaseInstructionLine::deconstruct((char*)wrong_instrs[i], ref);
        BOOST_REQUIRE_THROW(auto_ptr<SingleInstructionLine>(
                               new SingleInstructionLine(ref)),
                            InteruptionHandling);
    }
}
