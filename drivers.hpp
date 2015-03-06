#ifndef _BASE_DRIVERS_H_INCLUDED_
#define _BASE_DRIVERS_H_INCLUDED_

#include <string>
#include <map>

#include "targetdevice.hpp"


typedef enum {
    BASE_DRIVER_UNDEFINED,
    BASE_DRIVER_SERIAL,
    BASE_DRIVER_CAMERA
} driver_type_t;


class BaseDriver {
private:
    driver_type_t device_id;


    virtual driver_type_t id() throw() {
        return BASE_DRIVER_UNDEFINED;
    };

public:
    virtual ~BaseDriver() throw() {};
};


class SerialDriver: public BaseDriver, public TargetDeviceDriver {
private:
    std::string device_path;

public:
    SerialDriver(std::string path): device_path(path), TargetDeviceDriver(path) {};
    driver_type_t id() throw() {
        return BASE_DRIVER_SERIAL;
    }
    const std::string& path() const throw() {
        return device_path;
    }

    ~SerialDriver() throw() {};
};


#endif
