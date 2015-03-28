#include "confbind.hpp"


using namespace std;

Drivers::~Drivers() throw() {
    for(map<string, TargetDeviceDriver*>::iterator it = serials.begin();
        it != serials.end(); it++) {
        delete it->second;
    }
}


Drivers::Drivers(config_drivers_t *conf) {
    for(config_drivers_t::iterator it = conf->begin();
        it != conf->end(); it++) {
        SerialDriver *driver_conf = dynamic_cast<SerialDriver*>(it->second);
        if(driver_conf != NULL) {
            serials[it->first] = new TargetDeviceDriver(driver_conf->path());
        }
    }
}


TargetDeviceDriver *Drivers::serial(string name) {
    return serials.at(name);
}


DeviceSwitcher::DeviceSwitcher(Drivers &drivers, Switcher *conf) {
    this->relay_device = drivers.serial(conf->relay().driver);
    this->relay_port = conf->relay().port;
}


void DeviceSwitcher::turn_on() {
    relay_device->relay_set(this->relay_port, 1);
}


void DeviceSwitcher::turn_off() {
    relay_device->relay_set(this->relay_port, 1);
}


DeviceTemperature::DeviceTemperature(Drivers &drivers, Thermoswitcher *conf) {
    this->temperature_device = drivers.serial(conf->temperature().driver);
    this->adc_port = conf->temperature().port;
}


double DeviceTemperature::get_temperature() {
    int value = temperature_device->adc_get(this->adc_port);
    return value/1023.*5.;
}


Devices::Devices(Drivers &drivers, config_devices_t *conf) {
    for(config_devices_t::iterator it = conf->begin();
        it != conf->end(); it++) {
        switch(it->second->id()) {
        case DEVICE_SWITCHER: {
            Switcher *sw = dynamic_cast<Switcher*>(it->second);
            devices[it->first] =
                device_reference_t(new DeviceSwitcher(drivers, sw));
            break;
        }
        case DEVICE_THERMOSWITCHER: {
            Thermoswitcher *trm = dynamic_cast<Thermoswitcher*>(it->second);
            devices[it->first] =
                device_reference_t(new DeviceTemperature(drivers, trm));
            break;
        }
        case DEVICE_BOILER: {
            Boiler *blr = dynamic_cast<Boiler*>(it->second);
            devices[it->first] =
                device_reference_t(new DeviceBoiler(drivers, blr));
            break;
        }
        default:
            break;
        }
    }
}


device_reference_t& Devices::device(string name) {
    return devices.at(name);
}
