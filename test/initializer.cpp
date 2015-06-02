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
    conf = Config::get_from_struct(&rawconf);

    drivers = new TestDrivers(conf->drivers());
    devices = new Devices(*drivers, conf->devices());
}


ConfigInitializer::~ConfigInitializer() throw() {
    delete conf;
    delete devices;
    delete drivers;
}
