#ifndef _DEVICES_HPP_INCLUDED_
#define _DEVICES_HPP_INCLUDED_


typedef enum {
    DEVICE_UNDEFINED,
    DEVICE_BOILER,
    DEVICE_SWITCHER,
    DEVICE_THERMOSWITCHER
} device_type_t;


struct serial_link_t {
    std::string driver;
    int port;

    serial_link_t(std::string d, int p): driver(d), port(p) {};
    serial_link_t(std::pair<std::string, int> x): driver(x.first), port(x.second) {};
    operator std::pair<std::string, int>() {
        return std::pair<std::string, int>(driver, port);
    };
};


class BaseDevice {
public:

    virtual device_type_t id() const throw() {
        return DEVICE_UNDEFINED;
    }
    virtual ~BaseDevice() throw() {};
};


class Boiler: public BaseDevice {
private:
    serial_link_t _relay, _temperature;

public:
    Boiler(serial_link_t rel, serial_link_t temp): _relay(rel), _temperature(temp) {};

    device_type_t id() const throw() {
        return DEVICE_BOILER;
    };

    ~Boiler() throw() {};

    const serial_link_t& relay() const throw() {
        return _relay;
    }

    const serial_link_t& temperature() const throw() {
        return _temperature;
    }
};


class Switcher: public BaseDevice {
private:
    serial_link_t _relay;

public:
    Switcher(serial_link_t rel): _relay(rel) {};

    device_type_t id() const throw() {
        return DEVICE_SWITCHER;
    };

    ~Switcher() throw() {};

    const serial_link_t& relay() const throw() {
        return _relay;
    }
};


class Thermoswitcher: public BaseDevice {
private:
    serial_link_t _relay, _temperature;

public:
    Thermoswitcher(serial_link_t rel, serial_link_t temp): _relay(rel), _temperature(temp) {};

    device_type_t id() const throw() {
        return DEVICE_THERMOSWITCHER;
    };

    ~Thermoswitcher() throw() {};

    const serial_link_t& relay() const throw() {
        return _relay;
    }

    const serial_link_t& temperature() const throw() {
        return _temperature;
    }
};


#endif
