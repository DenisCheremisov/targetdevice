#include "confbind.hpp"


using namespace std;

Drivers::~Drivers() throw() {
    for(map<string, TargetDeviceDriver*>::iterator it = serials.begin();
        it != serials.end(); it++) {
        delete it->second;
    }
}


Drivers::Drivers(const config_drivers_t &conf) {
    for(config_drivers_t::const_iterator it = conf.begin();
        it != conf.end(); it++) {
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
    relay_device->relay_set(this->get_relay_port(), 1);
}


void DeviceSwitcher::turn_off() {
    relay_device->relay_set(this->relay_port, 1);
}


DeviceTemperature::DeviceTemperature(Drivers &drivers, Thermoswitcher *conf) {
    this->temperature_device = drivers.serial(conf->temperature().driver);
    this->adc_port = conf->temperature().port;
}


double DeviceTemperature::get_temperature() {
    int value = temperature_device->adc_get(this->get_adc_port());
    return value/1023.*5.;
}


Devices::~Devices() throw() {
    for(map<string, device_reference_t*>::iterator it = this->begin();
        it != this->end(); it++) {
        delete it->second;
    }
}


Devices::Devices(Drivers &drivers, const config_devices_t &conf) {
    for(config_devices_t::const_iterator it = conf.begin();
        it != conf.end(); it++) {
        switch(it->second->id()) {
        case DEVICE_SWITCHER: {
            Switcher *sw = dynamic_cast<Switcher*>(it->second);
            (*this)[it->first] =
                new device_reference_t(new DeviceSwitcher(drivers, sw));
            break;
        }
        case DEVICE_THERMOSWITCHER: {
            Thermoswitcher *trm = dynamic_cast<Thermoswitcher*>(it->second);
            (*this)[it->first] =
                new device_reference_t(new DeviceTemperature(drivers, trm));
            break;
        }
        case DEVICE_BOILER: {
            Boiler *blr = dynamic_cast<Boiler*>(it->second);
            (*this)[it->first] =
                new device_reference_t(new DeviceBoiler(drivers, blr));
            break;
        }
        default:
            break;
        }
    }
}


device_reference_t *Devices::device(string name) {
    return this->at(name);
}
