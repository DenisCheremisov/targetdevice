#ifndef _RUNTIME_HPP_INCLUDED_
#define _RUNTIME_HPP_INCLUDED_


#include <map>
#include <list>
#include <string>
#include <sstream>
#include <typeinfo>
#include <ctime>


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


template <typename T>
class NamedEntity: public std::map<std::string, T*> {
public:
    ~NamedEntity() throw() {
        for(typename NamedEntity<T>::iterator it = this->begin();
            it != this->end(); it++) {
            delete it->second;
        }
    }
};
typedef NamedEntity<Result> NamedResults;
typedef NamedEntity<Command> NamedCommands;


template <typename T>
class ListEntity: public std::list<T*> {
public:
    ~ListEntity() throw() {
        for(typename ListEntity<T>::iterator it = this->begin();
            it != this->end(); it++) {
            delete *it;
        }
    }
};
typedef ListEntity<Command> Commands;


class Executor {
private:
    Commands &commands;
    NamedCommands &named_commands;

public:
    Executor(Commands *cmds,
             NamedCommands *named_cmds):
        commands(*cmds), named_commands(*named_cmds) {};
    virtual ~Executor() throw() {};
    NamedResults *execute() throw();
};


class BaseTask {
public:
    virtual ~BaseTask() throw() {};

    virtual bool ready() throw() = 0;
    virtual bool expired() throw() = 0;
    virtual Command *get_command() throw() = 0;
};


class Schedule {
private:
    std::list<BaseTask*> items;

public:
    virtual ~Schedule() throw();
    Commands *get_commands();
    void add_item(BaseTask *item);
    void remove_expired();
};

//
// Task implementation

class TaskError: public std::exception, std::string {
public:
    ~TaskError() throw() {};
    TaskError(const std::string &msg): std::string(msg) {};
    const char *what() const throw() {
        return this->c_str();
    }
};


class TaskInPastError: public TaskError {
public:
    ~TaskInPastError() throw() {};
    TaskInPastError(const std::string &msg): TaskError(msg) {};
};


class DelayedTask: public BaseTask {
private:
    time_t point;
    bool is_expired;

public:
    virtual ~DelayedTask() throw() {};
    DelayedTask(time_t pnt) {
        time_t cur = time(0);
        if(difftime(pnt, cur) <= 0.) {
            throw TaskInPastError("Attempt to set delayed task in the past");
        }
        point = pnt;
        is_expired = false;
    }

    bool ready() throw() {
        time_t cur = time(0);
        if(cur >= point) {
            is_expired = true;
            return true;
        } else {
            return false;
        }
    }

    bool expired() throw() {
        return is_expired;
    }
};


class PermanentTask: public BaseTask {
private:
    bool is_ready;

public:
    virtual ~PermanentTask() throw() {};

    bool expired() throw() {
        return false;
    }
};

#endif
