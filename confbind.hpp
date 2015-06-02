#ifndef _GLOBALS_HPP_INCLUDED_
#define _GLOBALS_HPP_INCLUDED_

#include "confparser.hpp"
#include "targetdevice.hpp"

class Drivers {
protected:
    std::map<std::string, TargetDeviceDriver*> serials;

public:
    virtual ~Drivers() throw();
    Drivers() {};
    Drivers(const config_drivers_t &conf);

    TargetDeviceDriver* serial(std::string driver_name);
};


class BaseDescrDevice {
public:
    virtual ~BaseDescrDevice() throw() {};
};


class DeviceSwitcher: virtual public BaseDescrDevice {
protected:
    TargetDeviceDriver *relay_device;
    int relay_port;

public:
    ~DeviceSwitcher() throw() {};
    DeviceSwitcher(Drivers &drivers, Switcher *conf);

    int get_relay_port() {
        return relay_port;
    }

    virtual void turn_on();
    virtual void turn_off();
};


class DeviceTemperature: virtual public BaseDescrDevice {
protected:
    TargetDeviceDriver *temperature_device;
    int adc_port;
    float factor, shift;

public:
    ~DeviceTemperature() throw() {};
    DeviceTemperature(Drivers &drivers, Thermoswitcher *conf);

    int get_adc_port() {
        return adc_port;
    }

    virtual double get_temperature();
};


class DeviceBoiler: public DeviceSwitcher, public DeviceTemperature {
public:
    ~DeviceBoiler() throw() {};
    DeviceBoiler(Drivers &drivers,  Boiler *conf):
        DeviceSwitcher(drivers, conf),
        DeviceTemperature(drivers, conf) {};
};


struct device_reference_t {
public:
    BaseDescrDevice *basepointer;
    device_reference_t(BaseDescrDevice *swt): basepointer(swt) {};

    ~device_reference_t() throw() {
        delete basepointer;
    }
};


class Devices: public std::map<std::string, device_reference_t*> {
public:
    virtual ~Devices() throw();
    Devices(Drivers &drivers, const config_devices_t &conf);

    device_reference_t *device(std::string name);
};
#endif
