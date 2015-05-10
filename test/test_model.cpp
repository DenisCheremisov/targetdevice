#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ConfParserModule

#include <cstdio>
#include <memory>

#include <unistd.h>

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "../model.hpp"

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
        std::auto_ptr<YamlParser> parser(YamlParser::get(fp));
        ConfigStruct rawconf;
        yaml_parse(parser.get(), &rawconf);
        conf = Config::get_from_struct(&rawconf);

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


BOOST_AUTO_TEST_CASE(test_config_get) {
    ConfigInfoModel config_info;
    model_call_params_t params;
    params.config = init.conf;
    params.devices = init.devices;
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


BOOST_AUTO_TEST_CASE(test_couple_instruction) {
    s_map ref;
    BaseInstructionLine::deconstruct(
         "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:COUPLE=boiler.off:COUPLING-INTERVAL=40",
         ref);

    {
        CoupledInstructionLine instr(ref);
        BOOST_CHECK_EQUAL(instr.id, "1");
        BOOST_CHECK_EQUAL(instr.name, "boiler-on");
        BOOST_CHECK_EQUAL(instr.command, "boiler.on");
        BOOST_CHECK_EQUAL(instr.start, 12);
        BOOST_CHECK_EQUAL(instr.stop, -1);
        BOOST_CHECK_EQUAL(instr.restart, -1);
        BOOST_CHECK_EQUAL(instr.couple, "boiler.off");
        BOOST_CHECK_EQUAL(instr.coupling_interval, 40);
    }

    const int LENGTH = 2;
    const char *wrong_instrs[LENGTH] = {
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=16:RESTART=60:COUPLING-INTERVAL=a",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=16:RESTART=60:COUPLE=boiler.off",
    };

    for(int i = 0; i < LENGTH; i++) {
        ref.clear();
        BaseInstructionLine::deconstruct((char*)wrong_instrs[i], ref);
        BOOST_REQUIRE_THROW(auto_ptr<CoupledInstructionLine>(
                               new CoupledInstructionLine(ref)),
                            InteruptionHandling);
    }
}


BOOST_AUTO_TEST_CASE(test_parser_condition) {
    comparison_t res;

    res = parse_comparison("boiler.temperature.LT_80");
    BOOST_CHECK_EQUAL(res.source, "boiler");
    BOOST_CHECK_EQUAL(res.source_endpoint, ENDPOINT_TEMPERATURE);
    BOOST_CHECK_EQUAL(res.operation, COMPARISON_LT);
    BOOST_CHECK_EQUAL(res.value, 80);

    res = parse_comparison("boiler.temperature.LE_80");
    BOOST_CHECK_EQUAL(res.source, "boiler");
    BOOST_CHECK_EQUAL(res.source_endpoint, ENDPOINT_TEMPERATURE);
    BOOST_CHECK_EQUAL(res.operation, COMPARISON_LE);
    BOOST_CHECK_EQUAL(res.value, 80);

    res = parse_comparison("boiler.temperature.EQ_80");
    BOOST_CHECK_EQUAL(res.source, "boiler");
    BOOST_CHECK_EQUAL(res.source_endpoint, ENDPOINT_TEMPERATURE);
    BOOST_CHECK_EQUAL(res.operation, COMPARISON_EQ);
    BOOST_CHECK_EQUAL(res.value, 80);

    res = parse_comparison("boiler.temperature.GE_80");
    BOOST_CHECK_EQUAL(res.source, "boiler");
    BOOST_CHECK_EQUAL(res.source_endpoint, ENDPOINT_TEMPERATURE);
    BOOST_CHECK_EQUAL(res.operation, COMPARISON_GE);
    BOOST_CHECK_EQUAL(res.value, 80);

    res = parse_comparison("boiler.temperature.GT_80");
    BOOST_CHECK_EQUAL(res.source, "boiler");
    BOOST_CHECK_EQUAL(res.source_endpoint, ENDPOINT_TEMPERATURE);
    BOOST_CHECK_EQUAL(res.operation, COMPARISON_GT);
    BOOST_CHECK_EQUAL(res.value, 80);

    BOOST_REQUIRE_THROW(parse_comparison("boiler.camera.GT_80"),
                        InteruptionHandling);
    BOOST_REQUIRE_THROW(parse_comparison("boiler.camera.GT_80"),
                        InteruptionHandling);
    BOOST_REQUIRE_THROW(parse_comparison("boiler.temperature._80"),
                        InteruptionHandling);
    BOOST_REQUIRE_THROW(parse_comparison("boiler.temperature.K_80"),
                        InteruptionHandling);
    BOOST_REQUIRE_THROW(parse_comparison("boiler.temperature.EQ_"),
                        InteruptionHandling);
    BOOST_REQUIRE_THROW(parse_comparison("boiler.temperature."),
                        InteruptionHandling);
}


BOOST_AUTO_TEST_CASE(test_conditioned_instruction) {
    s_map ref;
    BaseInstructionLine::deconstruct(
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        ref);
    {
        ConditionInstructionLine instr(ref);
        BOOST_CHECK_EQUAL(instr.id, "1");
        BOOST_CHECK_EQUAL(instr.name, "boiler-on");
        BOOST_CHECK_EQUAL(instr.command, "boiler.on");
        BOOST_CHECK_EQUAL(instr.start, 12);
        BOOST_CHECK_EQUAL(instr.stop, -1);
        BOOST_CHECK_EQUAL(instr.couple, "boiler.off");
        BOOST_CHECK_EQUAL(instr.comparison.source, "boiler");
        BOOST_CHECK_EQUAL(instr.comparison.source_endpoint,
                          ENDPOINT_TEMPERATURE);
        BOOST_CHECK_EQUAL(instr.comparison.operation, COMPARISON_LT);
        BOOST_CHECK_EQUAL(instr.comparison.value, 80);
    }

    ref.clear();
    BaseInstructionLine::deconstruct(
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=40:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        ref);
    {
        ConditionInstructionLine instr(ref);
        BOOST_CHECK_EQUAL(instr.id, "1");
        BOOST_CHECK_EQUAL(instr.name, "boiler-on");
        BOOST_CHECK_EQUAL(instr.command, "boiler.on");
        BOOST_CHECK_EQUAL(instr.start, 12);
        BOOST_CHECK_EQUAL(instr.stop, 40);
        BOOST_CHECK_EQUAL(instr.couple, "boiler.off");
        BOOST_CHECK_EQUAL(instr.comparison.source, "boiler");
        BOOST_CHECK_EQUAL(instr.comparison.source_endpoint,
                          ENDPOINT_TEMPERATURE);
        BOOST_CHECK_EQUAL(instr.comparison.operation, COMPARISON_LT);
        BOOST_CHECK_EQUAL(instr.comparison.value, 80);
    }

    const int LENGTH = 9;
    const char *wrong_instrs[LENGTH] = {
        "NAME=boiler-on:COMMAND=boiler.on:START=12:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        "ID=1:COMMAND=boiler.on:START=12:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        "ID=1:NAME=boiler-on:START=12:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:CONDITION=boiler.temperature.LT_80",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:COUPLE=boiler.off",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=a:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=b:COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80",
        "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=12:STOP=40:COUPLE=boiler.off:CONDITION=boiler.temperature.80",
    };

    for(int i = 0; i < LENGTH; i++) {
        ref.clear();
        BaseInstructionLine::deconstruct((char*)wrong_instrs[i], ref);
        BOOST_REQUIRE_THROW(auto_ptr<CoupledInstructionLine>(
                               new CoupledInstructionLine(ref)),
                            InteruptionHandling);
    }
}


bool hasEnding (string fullString, string ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(),
                                         ending.length(), ending));
    } else {
        return false;
    }
}


BOOST_AUTO_TEST_CASE(test_command_generation) {
    ConfigInfoModel config_info;
    model_call_params_t params;
    params.config = init.conf;
    params.devices = init.devices;
    params.request_data =
        "ID=1:TYPE=value:COMMAND=switcher.on";

    auto_ptr<Command> cmd1(command_from_string(params, "switcher.on"));
    BOOST_CHECK(dynamic_cast<SwitcherOn*>(cmd1.get()) != NULL);

    auto_ptr<Command> cmd2(command_from_string(params, "switcher.off"));
    BOOST_CHECK(dynamic_cast<SwitcherOff*>(cmd2.get()) != NULL);

    auto_ptr<Command> cmd3(command_from_string(params,
                                               "temperature.temperature"));
    BOOST_CHECK(dynamic_cast<TemperatureGet*>(cmd3.get()) != NULL);

    auto_ptr<Command> cmd4(command_from_string(params, "boiler.on"));
    BOOST_CHECK(dynamic_cast<SwitcherOn*>(cmd4.get()) != NULL);

    auto_ptr<Command> cmd5(command_from_string(params, "boiler.off"));
    BOOST_CHECK(dynamic_cast<SwitcherOff*>(cmd5.get()) != NULL);

    BOOST_REQUIRE_THROW(
        auto_ptr<Command>(command_from_string(params, "boiler.of")),
        InteruptionHandling);

    BOOST_REQUIRE_THROW(
        auto_ptr<Command>(command_from_string(params, "switcher.temperature")),
        InteruptionHandling);

    BOOST_REQUIRE_THROW(
        auto_ptr<Command>(command_from_string(params, "ah.temperature")),
        InteruptionHandling);

    BOOST_REQUIRE_THROW(
        auto_ptr<Command>(command_from_string(params, "ah")),
        InteruptionHandling);
}


BOOST_AUTO_TEST_CASE(test_conditioned_schedule_getting) {
    model_call_params_t params;
    params.config = init.conf;
    params.devices = init.devices;
    params.request_data =
        "ID=1:TYPE=value:COMMAND=switcher.on";
    s_map ref;

    ref.clear();
    stringstream buf;
    buf << "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=";
    buf << time(NULL) + 4000;
    buf << ":STOP=";
    buf << time(NULL) + 40000;
    buf << ":COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80";
    BaseInstructionLine::deconstruct(buf.str(), ref);
    ConditionInstructionLine instr(ref);

    auto_ptr<BaseSchedule> cond_sched(get_conditioned(params, &instr));
    BOOST_CHECK(dynamic_cast<ConditionedSchedule*>(cond_sched.get()) !=
                (BaseSchedule*)NULL);

    ref.clear();
    buf.flush();
    buf << "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=";
    buf << time(NULL) - 4000;
    buf << ":STOP=";
    buf << time(NULL) + 40000;
    buf << ":COUPLE=boiler.off:CONDITION=boiler.temperature.LT_80";
    BaseInstructionLine::deconstruct(buf.str(), ref);
    ConditionInstructionLine instr1(ref);

    BOOST_REQUIRE_THROW(
        auto_ptr<BaseSchedule>((get_conditioned(params, &instr1))),
        ScheduleSetupError);
}


BOOST_AUTO_TEST_CASE(test_coupled_schedule_getting) {
    model_call_params_t params;
    params.config = init.conf;
    params.devices = init.devices;
    params.request_data =
        "ID=1:TYPE=value:COMMAND=switcher.on";
    s_map ref;

    ref.clear();
    stringstream buf;
    buf << "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=";
    buf << time(NULL) + 4000;
    buf << ":STOP=";
    buf << time(NULL) + 40000;
    buf << ":COUPLE=boiler.off:COUPLING-INTERVAL=100:RESTART=2000";
    BaseInstructionLine::deconstruct(buf.str(), ref);
    CoupledInstructionLine instr(ref);

    auto_ptr<BaseSchedule> cond_sched(get_coupled(params, &instr));
    BOOST_CHECK(dynamic_cast<CoupledCommandSchedule*>(cond_sched.get()) !=
                (BaseSchedule*)NULL);

    ref.clear();
    buf.flush();
    buf << "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=";
    buf << time(NULL) - 4000;
    buf << ":STOP=";
    buf << time(NULL) + 40000;
    buf << ":COUPLE=boiler.off:COUPLING-INTERVAL=100:RESTART=2000";
    BaseInstructionLine::deconstruct(buf.str(), ref);
    CoupledInstructionLine instr1(ref);

    BOOST_REQUIRE_THROW(
        auto_ptr<BaseSchedule>((get_coupled(params, &instr1))),
        ScheduleSetupError);
}


BOOST_AUTO_TEST_CASE(test_single_schedule_getting) {
    model_call_params_t params;
    params.config = init.conf;
    params.devices = init.devices;
    params.request_data =
        "ID=1:TYPE=value:COMMAND=switcher.on";
    s_map ref;

    ref.clear();
    stringstream buf;
    buf << "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=";
    buf << time(NULL) + 4000;
    buf << ":STOP=";
    buf << time(NULL) + 40000;
    buf << ":RESTART=2000";
    BaseInstructionLine::deconstruct(buf.str(), ref);
    SingleInstructionLine instr(ref);

    auto_ptr<BaseSchedule> cond_sched(get_single(params, &instr));
    BOOST_CHECK(dynamic_cast<SingleCommandSchedule*>(cond_sched.get()) !=
                (BaseSchedule*)NULL);

    ref.clear();
    buf.flush();
    buf << "ID=1:NAME=boiler-on:COMMAND=boiler.on:START=";
    buf << time(NULL) - 4000;
    buf << ":STOP=";
    buf << time(NULL) + 40000;
    buf << ":RESTART=2000";
    BaseInstructionLine::deconstruct(buf.str(), ref);
    SingleInstructionLine instr1(ref);

    BOOST_REQUIRE_THROW(
        auto_ptr<BaseSchedule>((get_single(params, &instr1))),
        ScheduleSetupError);
}


BOOST_AUTO_TEST_CASE(test_instruction_list_model) {
    model_call_params_t params;
    auto_ptr<NamedSchedule> sched(new NamedSchedule());
    params.config = init.conf;
    params.devices = init.devices;
    params.sched = sched.get();

    string req =
        "ID=0xfff0:TYPE=VALUE:COMMAND=temperature.temperature\n"
        "ID=0xfff1:TYPE=VALUE:COMMAND=temperature.temperature\n"
        "ID=0xfff2:TYPE=VALUE:COMMAND=boiler.temperature\n"
        "ID=0xfff3:TYPE=VALUE:COMMAND=boiler.temperature\n"
        "ID=1:TYPE=SINGLE:NAME=1:COMMAND=boiler.on:START=replaceit:RESTART=2000\n"
        "ID=2:TYPE=COUPLED:NAME=2:COMMAND=boiler.on:COUPLE=boiler.off:COUPLING-INTERVAL=500:START=replaceit:RESTART=2000\n"
        "ID=3:TYPE=CONDITION:NAME=3:COMMAND=boiler.on:COUPLE=boiler.off:START=replaceit:CONDITION=boiler.temperature.LT_50";
        ;
    stringstream buf;
    buf << time(NULL) + 4000;
    boost::replace_all(req, "replaceit", buf.str());
    params.request_data = req;

    InstructionListModel model;
    BOOST_CHECK_EQUAL(model.execute(params),
                      "ID=1:SUCCESS=1:VALUE=OK\n"
                      "ID=2:SUCCESS=1:VALUE=OK\n"
                      "ID=3:SUCCESS=1:VALUE=OK\n"
                      "ID=0xfff0:SUCCESS=1:VALUE=0\n"
                      "ID=0xfff1:SUCCESS=1:VALUE=0.00488759\n"
                      "ID=0xfff2:SUCCESS=1:VALUE=0\n"
                      "ID=0xfff3:SUCCESS=1:VALUE=0.00488759\n"
                      );

    BOOST_CHECK_EQUAL(params.sched->size(), 3);
    BOOST_CHECK(dynamic_cast<SingleCommandSchedule*>((*params.sched)["1"])
                != NULL);
    BOOST_CHECK(dynamic_cast<CoupledCommandSchedule*>((*params.sched)["2"])
                != NULL);
    BOOST_CHECK(dynamic_cast<ConditionedSchedule*>((*params.sched)["3"])
                != NULL);
}


BOOST_AUTO_TEST_CASE(test_wrong_instruction_list_model) {
    model_call_params_t params;
    auto_ptr<NamedSchedule> sched(new NamedSchedule());
    params.config = init.conf;
    params.devices = init.devices;
    params.sched = sched.get();

    string req =
        "ID=0xfff1:TYPE=VALUE:COMMAND=temperature.on\n"
        "ID=1:TYPE=SINGLE:NAME=1:COMMAND=boiler.on:START=replaceit:RESTART=2000\n"
        "ID=2:TYPE=COUPLED:NAME=2:COUPLE=boiler.off:COUPLING-INTERVAL=500:START=replaceit:RESTART=2000\n"
        "ID=3:TYPE=CONDITION:NAME=3:COMMAND=boiler.on:COUPLE=boiler.off:START=replaceit:CONDITION=boiler.temperature.LT_50";

    stringstream buf;
    buf << time(NULL) + 4000;
    boost::replace_all(req, "replaceit", buf.str());
    params.request_data = req;

    InstructionListModel model;
    BOOST_CHECK_EQUAL(
        model.execute(params),
        "ID=1:SUCCESS=1:VALUE=OK\n"
        "ID=3:SUCCESS=1:VALUE=OK\n"
        "ID=0xfff1:SUCCESS=0:ERROR=Device \"temperature\" does not support operation \"on\"\n"
        "ID=2:SUCCESS=0:ERROR=Key COMMAND required\n");
}
