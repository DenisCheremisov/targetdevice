#ifndef _BASE_DRIVERS_H_INCLUDED_
#define _BASE_DRIVERS_H_INCLUDED_

#include <string>
#include <map>

#include "targetdevice.hpp"


typedef enum {
    DRIVER_UNDEFINED,
    DRIVER_SERIAL,
    DRIVER_CAMERA
} driver_type_t;


class BaseDriver {
public:

    virtual driver_type_t id() throw() {
        return DRIVER_UNDEFINED;
    };
    virtual ~BaseDriver() throw() {};
};


class SerialDriver: public BaseDriver {
private:
    std::string device_path;

public:
    SerialDriver(std::string path): device_path(path) {};
    driver_type_t id() throw() {
        return DRIVER_SERIAL;
    }
    const std::string& path() const throw() {
        return device_path;
    }

    ~SerialDriver() throw() {};
};


#endif
