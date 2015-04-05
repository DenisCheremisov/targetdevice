#ifndef _CONDITIONS_HPP_INCLUDED_
#define _CONDITIONS_HPP_INCLUDED_

#include <map>
#include <string>

#include "confbind.hpp"


class TempIsLowerCondition: public BaseCondition {
private:
    DeviceTemperature *device;
    double bound;

public:
    TempIsLowerCondition(DeviceTemperature *dvc, double bnd):
        device(dvc), bound(bdn) {};
    ~TempIsLowerCondition() throw() {};

    bool indeed() {
        return device->get_temperature();
    }
};


class NoConditionError: public std::exception, public std::string {
public:
    ~NoConditionError() throw() {};
    NoConditionError(std::string message): std::string(message);

    const char *what() {
        return this->c_str();
    }
};

#endif
