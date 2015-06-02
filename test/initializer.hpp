#include "../confbind.hpp"



class TestDrivers: public Drivers {
public:
    ~TestDrivers() throw() {};
    TestDrivers(const config_drivers_t &conf);
};


class ConfigInitializer {
public:
    Config* conf;
    Devices* devices;
    Drivers *drivers;

    ConfigInitializer(const char *config_file_name);
    virtual ~ConfigInitializer();
};
