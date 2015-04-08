#ifndef _CONDITIONS_HPP_INCLUDED_
#define _CONDITIONS_HPP_INCLUDED_

#include <map>
#include <string>

#include "confbind.hpp"


template <typename T>
bool compare_lt(T a, T b) {
    return a < b;
}


template <typename T>
bool compare_le(T a, T b) {
    return a <= b;
}


template <typename T>
bool compare_eq(T a, T b) {
    return a == b;
}


template <typename T>
bool compare_ge(T a, T b) {
    return a >= b;
}


template <typename T>
bool compare_gt(T a, T b) {
    return a > b;
}


class TemperatureCondition: public BaseCondition {
private:
    DeviceTemperature *device;
    double bound;
    bool (*comparator)(double, double);

public:
    TemperatureCondition(DeviceTemperature *dvc, double bnd,
                         bool (*cmprtr)(double, double)):
        device(dvc), bound(bnd), comparator(cmprtr) {};
    ~TemperatureCondition() throw() {};

    bool indeed() {
        return comparator(device->get_temperature(), bound);
    }
};


class NoConditionError: public std::exception, public std::string {
public:
    ~NoConditionError() throw() {};
    NoConditionError(std::string message): std::string(message) {};

    const char *what() {
        return this->c_str();
    }
};

#endif
