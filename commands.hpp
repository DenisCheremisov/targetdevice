#ifndef _COMMANDS_HPP_INCLUDED_
#define _COMMANDS_HPP_INCLUDED_

#include "targetdevice.hpp"
#include "runtime.hpp"
#include "confbind.hpp"


class SwitcherOn: public Command {
private:
    DeviceSwitcher *device;
public:
    ~SwitcherOn() throw() {};
    SwitcherOn(DeviceSwitcher *dvc): device(dvc) {};

    Result *execute() throw();
};


class SwitcherOff: public Command {
private:
    DeviceSwitcher *device;
public:
    ~SwitcherOff() throw() {};
    SwitcherOff(DeviceSwitcher *dvc): device(dvc) {};

    Result *execute() throw();
};


class TemperatureGet: public Command {
private:
    DeviceTemperature *device;

public:
    ~TemperatureGet() throw() {};
    TemperatureGet(DeviceTemperature *dvc): device(dvc) {}

    Result *execute() throw();
};


class BoilerOn: public Command {
private:
    DeviceBoiler *device;

public:
    ~BoilerOn() throw() {};
    BoilerOn(DeviceBoiler* dvc): device(dvc) {};

    Result *execute() throw();
};


class BoilerOff: public Command {
private:
    DeviceBoiler *device;

public:
    ~BoilerOff() throw() {};
    BoilerOff(DeviceBoiler* dvc): device(dvc) {};

    Result *execute() throw();
};


class BoilerTemperatureGet: public Command {
private:
    DeviceBoiler *device;

public:
    ~BoilerTemperatureGet() throw() {};
    BoilerTemperatureGet(DeviceBoiler* dvc): device(dvc) {};

    Result *execute() throw();
};

#endif
