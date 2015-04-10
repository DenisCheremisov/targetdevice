#ifndef _COMMANDS_HPP_INCLUDED_
#define _COMMANDS_HPP_INCLUDED_

#include "targetdevice.hpp"
#include "confbind.hpp"
#include "runtime.hpp"
#include "confbind.hpp"


class CommandSetupError: public std::exception, public std::string {
public:
    ~CommandSetupError() throw() {};
    CommandSetupError(std::string msg): std::string(msg) {};

    const char *what() const throw() {
        return c_str();
    }
};


class SwitcherOn: public Command {
private:
    DeviceSwitcher *device;
public:
    ~SwitcherOn() throw() {};
    SwitcherOn(device_reference_t *ref);

    Result *execute() throw();
};


class SwitcherOff: public Command {
private:
    DeviceSwitcher *device;
public:
    ~SwitcherOff() throw() {};
    SwitcherOff(device_reference_t *ref);

    Result *execute() throw();
};


class TemperatureGet: public Command {
private:
    DeviceTemperature *device;

public:
    ~TemperatureGet() throw() {};
    TemperatureGet(device_reference_t *ref);

    Result *execute() throw();
};

#endif
