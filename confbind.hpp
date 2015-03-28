#ifndef _GLOBALS_HPP_INCLUDED_
#define _GLOBALS_HPP_INCLUDED_


#include "confparser.hpp"
#include "targetdevice.hpp"

class Drivers {
private:
    std::map<std::string, TargetDeviceDriver*> serials;

public:
    virtual ~Drivers() throw();
    Drivers(config_drivers_t *conf);

    TargetDeviceDriver* serial(std::string driver_name);
};


class DeviceSwitcher {
private:
    TargetDeviceDriver *relay_device;
    int relay_port;

public:
    DeviceSwitcher(Drivers &drivers, Switcher *conf);

    void turn_on();
    void turn_off();
};


class DeviceTemperature {
private:
    TargetDeviceDriver *temperature_device;
    int adc_port;

public:
    DeviceTemperature(Drivers &drivers, Thermoswitcher *conf);

    double get_temperature();
};


class DeviceBoiler: public DeviceSwitcher, public DeviceTemperature {
public:
    ~DeviceBoiler() throw() {};
    DeviceBoiler(Drivers &drivers,  Boiler *conf);
};


struct device_reference_t {
public:
    device_type_t type;
    union {
        DeviceSwitcher *switcher;
        DeviceTemperature *temperature;
        DeviceBoiler *boiler;
    };
    void (device_reference_t::*destructor) ();

#define DESTRUCTOR(member, type) throw()                \
    { if(member != (type*)NULL) delete member; }

    void destruct_switcher() DESTRUCTOR(switcher, DeviceSwitcher);
    void destruct_temperature() DESTRUCTOR(temperature, DeviceTemperature);
    void destruct_boiler() DESTRUCTOR(boiler, DeviceBoiler);
    void destructor_void() throw() {};
#undef DESTRUCTOR

    device_reference_t():
        type(DEVICE_UNDEFINED),
        destructor(&device_reference_t::destructor_void) {};
    // device_reference_t(const device_reference_t &d):
    //     type(d.type),
    //     switcher(d.switcher),
    //     destructor(d.destructor) {};
    device_reference_t(DeviceSwitcher *swt): type(DEVICE_BOILER),
                                             switcher(swt),
                                             destructor(&device_reference_t::destruct_switcher) {};
    device_reference_t(DeviceTemperature *temp): type(DEVICE_THERMOSWITCHER),
                                                 temperature(temp),
                                                 destructor(&device_reference_t::destruct_temperature) {};
    device_reference_t(DeviceBoiler *blr): type(DEVICE_BOILER),
                                           boiler(blr),
                                           destructor(&device_reference_t::destruct_boiler) {};

    ~device_reference_t() throw() {
        ((*this).*destructor)();
    }
};


class Devices {
private:
    std::map<std::string, device_reference_t> devices;

public:
    virtual ~Devices() throw() {};
    Devices(Drivers &drivers, config_devices_t *conf);

    device_reference_t& device(std::string name);
};
#endif
