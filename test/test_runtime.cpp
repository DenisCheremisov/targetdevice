#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE RuntimeModule

#include <cstdio>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "../runtime.hpp"

using namespace std;


class TestCommand2: public Command {
public:
    static int counter;

    Result *execute() throw() {
        return new IntResult(TestCommand2::counter++);
    }
};
int TestCommand2::counter = 0;


class TestCommand: public Command {
public:
    static int counter;

    ~TestCommand() throw() {
        TestCommand2::counter++;
    }

    Result *execute() throw() {
        return new IntResult(TestCommand::counter++);
    }
};
int TestCommand::counter = 0;


time_t future() {
    return time(NULL) + 1000;
}


BOOST_AUTO_TEST_CASE(test_runtime_executor) {
    Commands res;
    for(int i = 0; i < 10; i++) {
        res.push_back(new TestCommand);
    }

    Executor exec(&res);
    TestCommand::counter = 0;
    Results *out = exec.execute();
    int i = 0;
    for(Results::iterator it = out->begin();
        it != out->end(); it++, i++) {
        BOOST_CHECK_EQUAL(atol((**it).value().c_str()), i);
    }

    for(Commands::iterator it = res.begin();
        it != res.end(); it++) {
        delete *it;
    }
    delete out;
}


BOOST_AUTO_TEST_CASE(test_single_command) {
    time_t ftr = future();

    SingleCommandSchedule sched(new TestCommand, ftr, -1);
    auto_ptr<Command> command(new TestCommand);
    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    Commands *res = sched.get_commands(ftr + 1);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;
    BOOST_CHECK_EQUAL(sched.is_expired(), true);
    res = sched.get_commands(ftr + 1000);
    BOOST_CHECK_EQUAL(res->size(), 0);
    delete res;
}


BOOST_AUTO_TEST_CASE(test_single_command_forever) {
    time_t ftr = future();

    SingleCommandSchedule sched(new TestCommand, ftr, -1, 1000);
    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    Commands *res = sched.get_commands(ftr + 1);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    res = sched.get_commands(ftr + 500);
    BOOST_CHECK_EQUAL(res->size(), 0);
    delete res;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    res = sched.get_commands(ftr + 1500);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;
}


BOOST_AUTO_TEST_CASE(test_coupled_command) {
    CoupledCommandSchedule sched(new TestCommand, 0, -1,
                                 new TestCommand2, 600);
    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    Commands *res = sched.get_commands(1);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    res = sched.get_commands(1000);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;

    BOOST_CHECK(sched.is_expired());
}


BOOST_AUTO_TEST_CASE(test_coupled_command_forever) {
    CoupledCommandSchedule sched(new TestCommand, 0, -1,
                                 new TestCommand2, 600, 1000);
    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    Commands *res = sched.get_commands(1);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    res = sched.get_commands(700);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    res = sched.get_commands(900);
    BOOST_CHECK_EQUAL(res->size(), 0);
    delete res;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    res = sched.get_commands(1100);
    BOOST_CHECK_EQUAL(res->size(), 1);
    delete res;
}


BOOST_AUTO_TEST_CASE(test_coupled_urgent_exit) {
    CoupledCommandSchedule
        *sched = new CoupledCommandSchedule(new TestCommand, 0, -1,
                                            new TestCommand2, 600, 1000);
    TestCommand::counter = 0;
    TestCommand2::counter = 0;

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    auto_ptr<Commands> res(sched->get_commands(1));
    BOOST_CHECK_EQUAL(res->size(), 1);
    auto_ptr<Executor> exec(new Executor(res.get()));
    delete exec->execute();

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    BOOST_CHECK_EQUAL(TestCommand2::counter, 0);
    delete sched;
    BOOST_CHECK_EQUAL(TestCommand2::counter, 1);
}


class Condition: public BaseCondition {
private:
    bool is_indeed;

public:
    ~Condition() throw() {};
    Condition(): is_indeed(false) {};
    bool indeed() {
        is_indeed = !is_indeed;
        return is_indeed;
    }
};


BOOST_AUTO_TEST_CASE(test_conditioned_command) {
    time_t ftr = future();
    auto_ptr<Command> cmd(new TestCommand), cmd2(new TestCommand2);
    auto_ptr<Condition> cnd(new Condition);

    ConditionedSchedule sched(new TestCommand, new TestCommand2,
                              new Condition, ftr, -1);
    TestCommand::counter = 0;
    TestCommand2::counter = 0;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    Commands *res = sched.get_commands(ftr + 1);
    BOOST_CHECK_EQUAL(res->size(), 1);
    {
        auto_ptr<Executor> exec(new Executor(res));
        delete exec->execute();
    }
    delete res;
    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    res = sched.get_commands(ftr + 1000);
    BOOST_CHECK_EQUAL(res->size(), 1);
    {
        auto_ptr<Executor> exec(new Executor(res));
        delete exec->execute();
    }
    delete res;

    BOOST_CHECK_EQUAL(TestCommand::counter, 1);
    BOOST_CHECK_EQUAL(TestCommand2::counter, 1);
}


BOOST_AUTO_TEST_CASE(test_conditioned_command_forever) {
    time_t ftr = future();
    ConditionedSchedule sched(new TestCommand, new TestCommand2,
                              new Condition, ftr, -1);

    TestCommand::counter = 0;
    TestCommand2::counter = 0;
    const int LIMIT = 5;

    for(int i = 0; i < LIMIT; i++) {
        BOOST_CHECK_EQUAL(sched.is_expired(), false);
        Commands *res = sched.get_commands(ftr + 1);
        BOOST_CHECK_EQUAL(res->size(), 1);
        {
            auto_ptr<Executor> exec(new Executor(res));
            delete exec->execute();
        }
        delete res;
        BOOST_CHECK_EQUAL(sched.is_expired(), false);
        res = sched.get_commands(ftr + 1000);
        BOOST_CHECK_EQUAL(res->size(), 1);
        {
            auto_ptr<Executor> exec(new Executor(res));
            delete exec->execute();
        }
        delete res;
    }

    BOOST_CHECK_EQUAL(TestCommand::counter, LIMIT);
    BOOST_CHECK_EQUAL(TestCommand2::counter, LIMIT);
}


BOOST_AUTO_TEST_CASE(test_conditioned_urgent_exit) {
    ConditionedSchedule
        *sched = new ConditionedSchedule(new TestCommand,
                                         new TestCommand2,
                                         new Condition,
                                         0, 1000);
    TestCommand::counter = 0;
    TestCommand2::counter = 0;

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    auto_ptr<Commands> res(sched->get_commands(1));
    BOOST_CHECK_EQUAL(res->size(), 1);
    auto_ptr<Executor> exec(new Executor(res.get()));
    delete exec->execute();

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    BOOST_CHECK_EQUAL(TestCommand2::counter, 0);
    delete sched;
    BOOST_CHECK_EQUAL(TestCommand2::counter, 1);
}


BOOST_AUTO_TEST_CASE(test_stop_point_single) {
    time_t ftr = future();

    auto_ptr<SingleCommandSchedule>
        sched(new SingleCommandSchedule(new TestCommand,
                                        ftr, ftr + 500, 200));

    TestCommand::counter = 0;
    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 1));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 201));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 401));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 601));
        BOOST_CHECK_EQUAL(res->size(), 0);
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), true);
}


BOOST_AUTO_TEST_CASE(test_stop_point_couple) {
    time_t ftr = future();

    auto_ptr<SingleCommandSchedule>
        sched(new CoupledCommandSchedule(new TestCommand,
                                         ftr, ftr + 300,
                                         new TestCommand2, 100, 200));

    TestCommand::counter = 0;
    TestCommand2::counter = 0;
    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 1));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 101));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 201));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 301));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 401));
        BOOST_CHECK_EQUAL(res->size(), 0);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), true);
    BOOST_CHECK_EQUAL(TestCommand::counter, 2);
    BOOST_CHECK_EQUAL(TestCommand2::counter, 2);
}


BOOST_AUTO_TEST_CASE(test_stop_point_conditioned) {
    time_t ftr = future();

    auto_ptr<ConditionedSchedule>
        sched(new ConditionedSchedule(new TestCommand,
                                      new TestCommand2,
                                      new Condition,
                                      ftr, ftr + 100));

    TestCommand::counter = 0;
    TestCommand2::counter = 0;
    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 1));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 101));
        BOOST_CHECK_EQUAL(res->size(), 1);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), false);
    {
        auto_ptr<Commands> res(sched->get_commands(ftr + 201));
        BOOST_CHECK_EQUAL(res->size(), 0);
        auto_ptr<Executor> exec(new Executor(res.get()));
        delete exec->execute();
    }

    BOOST_CHECK_EQUAL(sched->is_expired(), true);
    BOOST_CHECK_EQUAL(TestCommand::counter, 1);
    BOOST_CHECK_EQUAL(TestCommand2::counter, 1);
}



BOOST_AUTO_TEST_CASE(test_list_schedule) {
    ListSchedule sched;
    time_t ftr = future();

    for(int i = 0; i < 6; i++) {
        sched << new SingleCommandSchedule(new TestCommand, ftr, -1);
        sched << new SingleCommandSchedule(new TestCommand, ftr, -1, 600);
    }

    Commands *nores;

    TestCommand::counter = 0;
    nores = sched.get_commands(ftr + 100);
    BOOST_CHECK_EQUAL(nores->size(), 12);
    Executor *exec = new Executor(nores);
    delete exec->execute();
    BOOST_CHECK_EQUAL(TestCommand::counter, 12);
    delete exec;
    delete nores;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    nores = sched.get_commands(ftr + 700);
    BOOST_CHECK_EQUAL(nores->size(), 6);
    exec = new Executor(nores);
    delete exec->execute();
    BOOST_CHECK_EQUAL(TestCommand::counter, 18);
    delete exec;
    delete nores;
}


BOOST_AUTO_TEST_CASE(test_general_schedule) {
    time_t ftr = future();
    ListSchedule *s1, *s2, *s3, *s4;
    s1 = new ListSchedule;
    s2 = new ListSchedule;
    s3 = new ListSchedule;
    s4 = new ListSchedule;
    for(int i = 0; i < 6; i++) {
        *s1 << new SingleCommandSchedule(new TestCommand, ftr, -1);
        *s1 << new SingleCommandSchedule(new TestCommand, ftr, -1, 600);
        *s2 << new SingleCommandSchedule(new TestCommand, ftr, -1);
        *s2 << new SingleCommandSchedule(new TestCommand, ftr, -1, 600);
        *s3 << new SingleCommandSchedule(new TestCommand, ftr, -1);
        *s3 << new SingleCommandSchedule(new TestCommand, ftr, -1, 600);
        *s4 << new SingleCommandSchedule(new TestCommand2, ftr, -1);
    }

    NamedSchedule sched;
    sched
        .set_schedule("opt1", s1)
        .set_schedule("opt2", s2)
        .set_schedule("opt3", s3);

    Commands *nores;

    TestCommand::counter = 0;
    TestCommand2::counter = 0;
    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    nores = sched.get_commands(ftr + 100);
    BOOST_CHECK_EQUAL(nores->size(), 12*3);
    Executor *exec = new Executor(nores);
    delete exec->execute();
    BOOST_CHECK_EQUAL(TestCommand::counter, 12*3);
    delete exec;
    delete nores;

    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    sched.set_schedule("opt4", s4);
    nores = sched.get_commands(ftr + 1000);
    BOOST_CHECK_EQUAL(nores->size(), 6*3 + 6);
    exec = new Executor(nores);
    delete exec->execute();
    BOOST_CHECK_EQUAL(TestCommand::counter, 12*3 + 6*3);
    BOOST_CHECK_EQUAL(TestCommand2::counter, 24);
    delete exec;
    delete nores;

    TestCommand::counter = 0;
    TestCommand2::counter = 0;
    BOOST_CHECK_EQUAL(sched.is_expired(), false);
    nores = sched.get_commands(ftr + 1200);
    BOOST_CHECK_EQUAL(nores->size(), 6*3);
    exec = new Executor(nores);
    delete exec->execute();
    BOOST_CHECK_EQUAL(TestCommand::counter, 6*3);
    BOOST_CHECK_EQUAL(TestCommand2::counter, 0);
    delete exec;
    delete nores;
}
