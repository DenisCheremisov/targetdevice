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
    time_t start, stop;
    int restart;

    ~SingleInstructionLine() throw() {};
    SingleInstructionLine(s_map&);
};


class CoupledInstructionLine: public SingleInstructionLine {
public:
    std::string couple;
    int coupling_interval;

    ~CoupledInstructionLine() throw() {};
    CoupledInstructionLine(s_map&);
};


typedef enum {
        COMPARISON_LT,
        COMPARISON_LTE,
        COMPARISON_EQ,
        COMPARISON_GTE,
        COMPARISON_GT
} operation_t;


typedef enum {
    ENDPOINT_TEMPERATURE
} source_endpoint_t;


struct comparison_t {
    std::string source;
    source_endpoint_t source_endpoint;
    operation_t operation;
    double value;
};


comparison_t parse_comparison(std::string source) throw(InteruptionHandling);


class ConditionInstructionLine: public BaseInstructionLine {
public:
    std::string id, command, name, couple;
    time_t start, stop;
    comparison_t comparison;

    ~ConditionInstructionLine() throw() {};
    ConditionInstructionLine(s_map&);
};


#endif
