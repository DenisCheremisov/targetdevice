#ifndef _MODEL_HPP_DEFINED_
#define _MODEL_HPP_DEFINED_

#include "confbind.hpp"
#include "controller.hpp"

struct model_call_params_t {
    Config *config;
    Devices *devices;
    std::string request_data;
};


class BaseModel {
public:
    virtual ~BaseModel() throw() {}

    virtual std::string execute(model_call_params_t &params)
        throw(InteruptionHandling) = 0;
};


class StopChainModel: public BaseModel {
public:
    ~StopChainModel() throw() {};
    std::string execute(model_call_params_t &params)
        throw(InteruptionHandling) {
        throw InteruptionHandling("OK");
    }
};


class ConfigInfoModel: public BaseModel {
public:
    ~ConfigInfoModel() throw() {};

    std::string execute(model_call_params_t &params)
        throw(InteruptionHandling);
};


typedef std::map<std::string, std::string> s_map;

class BaseInstructionLine {
public:
    enum {
        INSTRUCTION_VALUE,
        INSTRUCTION_SINGLE,
        INSTRUCTION_COUPLED,
        INSTRUCTION_CONDITIONAL
    } type;

    virtual ~BaseInstructionLine() throw() {};
    virtual void build(std::string) {};

    static void deconstruct(std::string, s_map&);
};


class ValueInstructionLine: public BaseInstructionLine {
public:
    std::string id, command;

    virtual ~ValueInstructionLine() throw() {};
    ValueInstructionLine(s_map&);
};


class SingleInstructionLine: public BaseInstructionLine {
public:
    std::string id, command, name;
    time_t start, stop, restart;

    ~SingleInstructionLine() throw() {};
    SingleInstructionLine(s_map&);
};


class CoupledInstructionLine: public BaseInstructionLine {
public:
    std::string id, command, name, couple;
    time_t start, stop, coupling_interval, command_restart;

    ~CoupledInstructionLine() throw() {};
    CoupledInstructionLine(s_map&);
};


struct comparison_t {
    std::string source;
    enum {
        TEMPERATURE
    } source_endpoint;
    enum {
        COMPARISON_LT,
        COMPARISON, LTE,
        COMPARISON_GTE,
        COMPARISON_GT,
        COMPARISON_EQ
    } operation;
    double value;
};


class ConditionInstructionLine: public BaseInstructionLine {
public:
    std::string id, command, name, couple;
    time_t start, stop;
    comparison_t comparison;

    ~ConditionInstructionLine() throw() {};
    ConditionInstructionLine(s_map&);
};


#endif
