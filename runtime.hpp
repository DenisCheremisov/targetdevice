#ifndef _RUNTIME_HPP_INCLUDED_
#define _RUNTIME_HPP_INCLUDED_


#include <map>
#include <list>
#include <string>
#include <sstream>
#include <typeinfo>
#include <ctime>
#include <pthread.h>


const int RUNTIME_WAKE_PAUSE = 5; //


typedef enum {
    RESULT_SERIAL_ERROR,
    RESULT_SERIAL_WRONGLINE
} error_result_t;


class Result {
public:
    virtual ~Result() throw() {};
    virtual std::string value() throw() = 0;
};


class ErrorResult: public Result {
private:
    std::string message;
    error_result_t error_code;

public:
    ErrorResult(error_result_t err_code,
                std::string msg):
        error_code(err_code), message(msg) {};
    virtual ~ErrorResult() throw() {};

    std::string value() throw() {
        return message;
    };
    error_result_t code() throw() {
        return error_code;
    };
};


class StringResult: public Result, public std::string {
public:
    StringResult(const std::string &msg): std::string(msg) {};
    virtual ~StringResult() throw() {};

    std::string value() throw() {
        return *this;
    };
};


template <class T> class ParametrizedResult: public Result {
private:
    T _value;

public:
    ParametrizedResult(T res_value): _value(res_value) {};
    virtual ~ParametrizedResult() throw() {};

    std::string value() throw() {
        std::stringstream buf;
        buf << _value;
        return buf.str();
    };
};


typedef ParametrizedResult<int> IntResult;
typedef ParametrizedResult<double> FloatResult;


class Command {
public:
    virtual ~Command() throw() {};
    virtual Result *execute() throw() = 0;
};


class Results: public std::list<Result*> {
public:
    ~Results() throw() {
        for(Results::iterator it = this->begin(); it != this->end(); it++) {
            delete *it;
        }
    }
};

typedef std::list<Command*> Commands;


class Executor {
private:
    Commands &commands;

public:
    virtual ~Executor() throw() {};
    Results *execute() throw();
    Executor(Commands *cmds): commands(*cmds) {};
};


class ScheduleError: public std::exception, std::string {
public:
    ~ScheduleError() throw() {};
    ScheduleError(const std::string &msg): std::string(msg) {};
    const char *what() const throw() {
        return this->c_str();
    }
};


class ScheduleSetupError: public ScheduleError {
public:
    ~ScheduleSetupError() throw() {};
    ScheduleSetupError(const std::string &msg): ScheduleError(msg) {};
};


class BaseSchedule {
public:
    virtual ~BaseSchedule() throw() {};
    virtual Commands *get_commands(time_t tm) = 0;
    virtual bool is_expired() = 0;
};



class ListSchedule: public BaseSchedule, protected std::list<BaseSchedule*> {
public:
    virtual ~ListSchedule() throw();
    Commands *get_commands(time_t tm);
    ListSchedule& operator<<(BaseSchedule *item);
    bool is_expired();
};


class NamedSchedule: public BaseSchedule,
                     protected std::map<std::string, BaseSchedule*> {
public:
    virtual ~NamedSchedule() throw();
    Commands *get_commands(time_t tm);
    NamedSchedule& set_schedule(std::string name, BaseSchedule *sched);
    bool is_expired();
};


class SingleCommandSchedule: public BaseSchedule {
private:
    Command *command;
    time_t start_point;
    time_t stop_point;
    int restart_period;
    bool expired;

public:
    ~SingleCommandSchedule() throw() {
        delete command;
    }

    SingleCommandSchedule(Command *cmd, time_t start_point, time_t stop_point,
                          int restart_period = -1);

    Commands *get_commands(time_t tm);
    time_t get_start_point() {
        return start_point;
    };
    bool is_expired();
};


class CoupledCommandSchedule: public SingleCommandSchedule {
private:
    Command *coupled_command;
    time_t coupled_point;
    int coupled_interval;
    bool on_coupling;

public:
    ~CoupledCommandSchedule() throw ();

    CoupledCommandSchedule(Command *cmd,
                           time_t start_point, time_t stop_point,
                           Command *coupled_command,
                           int coupled_interval,
                           int restart_period = -1);

    Commands *get_commands(time_t tm);
    bool is_expired();
};


class BaseCondition {
public:
    virtual ~BaseCondition() throw() {};
    virtual bool indeed() = 0;
};


class ConditionedSchedule: public BaseSchedule {
private:
    Command *command, *coupled_command;
    BaseCondition *condition;
    time_t start_point, stop_point;
    bool to_be_stopped;
    bool expired;

public:
    ~ConditionedSchedule() throw();

    ConditionedSchedule(Command *cmd,
                        Command *coupled_cmd,
                        BaseCondition *cnd,
                        time_t start_point = -1, time_t stop_point = -1);
    Commands *get_commands(time_t tm);
    bool is_expired();
};


#endif
