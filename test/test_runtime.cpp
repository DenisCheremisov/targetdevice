#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE RuntimeModule

#include <cstdio>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "../runtime.hpp"

using namespace std;


class TestCommand: public Command {
public:
    static int counter;

    Result *execute() throw() {
        return new IntResult(TestCommand::counter++);
    }
};
int TestCommand::counter = 0;


BOOST_AUTO_TEST_CASE(test_runtime_executor) {
    Commands nores;
    for(int i = 0; i < 10; i++) {
        nores.push_back(new TestCommand);
    }
    NamedCommands res;
    res["1"] = new TestCommand;
    res["2"] = new TestCommand;
    res["3"] = new TestCommand;

    Executor exec(&nores, &res);
    TestCommand::counter = 0;
    NamedResults *out = exec.execute();

    BOOST_CHECK_EQUAL(out->at("1")->value(), "10");
    BOOST_CHECK_EQUAL(out->at("2")->value(), "11");
    BOOST_CHECK_EQUAL(out->at("3")->value(), "12");

    delete out;
}


class EvenTask: public BaseTask {
public:
    ~EvenTask() throw() {};

    bool ready() throw() {
        return true;
    }

    bool expired() throw() {
        return false;
    }

    Command *get_command() throw() {
        return new TestCommand;
    }
};


class OddTask: public EvenTask {
public:
    ~OddTask() throw() {};

    bool expired() throw() {
        return true;
    }
};


BOOST_AUTO_TEST_CASE(test_schedule) {
    Schedule sched;

    for(int i = 0; i < 6; i++) {
        sched.add_item(new EvenTask);
        sched.add_item(new OddTask);
    }

    Commands *nores;
    NamedCommands res;

    TestCommand::counter = 0;
    nores = sched.get_commands();
    BOOST_CHECK_EQUAL(nores->size(), 12);
    Executor *exec = new Executor(nores, &res);
    delete exec->execute();
    BOOST_CHECK_EQUAL(TestCommand::counter, 12);
    delete exec;
    delete nores;

    sched.remove_expired();
    nores = sched.get_commands();
    BOOST_CHECK_EQUAL(nores->size(), 6);
    exec = new Executor(nores, &res);
    delete exec->execute();
    BOOST_CHECK_EQUAL(TestCommand::counter, 18);
    delete exec;
    delete nores;
}
