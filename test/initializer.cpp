#include "initializer.hpp"
#include "drivers.hpp"
#include "../confparser.hpp"

#include <cstdio>
#include <memory>


TestDrivers::TestDrivers(const config_drivers_t &conf) {
    for(auto &kv: conf) {
        auto *driver_conf = dynamic_cast<SerialDriver*>(kv.second);
        if(driver_conf != NULL) {
            serials[kv.first] =
                new TargetDeviceDriver(new TestSerialCommunicator());
        }
    }
}


ConfigInitializer::ConfigInitializer(const char *config_file_name) {
    FILE *fp = fopen(config_file_name, "r");
    if(fp == NULL) {
        perror(config_file_name);
        throw std::runtime_error(
            std::string("Cannot open sample file: ") + config_file_name);
    }
    std::unique_ptr<YamlParser> parser(YamlParser::get(fp));
    ConfigStruct rawconf;
    yaml_parse(parser.get(), &rawconf);

    // Setting up resources
    resources = new Resources;
    for(DevicesStruct::iterator it = rawconf.devices.begin();
        it != rawconf.devices.end(); it++) {
        SerialDeviceStruct *device = dynamic_cast<SerialDeviceStruct*>(
            it->second);
        if(device == NULL) {
            continue;
        }
        if(device->type.value == "switcher") {
            resources->add_resource(it->first + ".on", it->first);
            resources->add_resource(it->first + ".off", it->first);
        } else if(device->type.value == "boiler") {
            resources->add_resource(it->first + ".on", it->first);
            resources->add_resource(it->first + ".off", it->first);
        }
    }

    conf = Config::get_from_struct(&rawconf);

    drivers = new TestDrivers(conf->drivers());
    devices = new Devices(*drivers, conf->devices());
}


ConfigInitializer::~ConfigInitializer() throw() {
    delete conf;
    delete devices;
    delete drivers;
    delete resources;
}
