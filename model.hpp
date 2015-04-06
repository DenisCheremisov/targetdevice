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


#endif
