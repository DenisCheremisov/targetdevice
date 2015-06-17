#ifndef _CONFPARSER_H_INCLUDED_
#define _CONFPARSER_H_INCLUDED_

#include <cstdio>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <ctime>

#include <yaml.h>


#include "yamlparser.hpp"
#include "drivers.hpp"
#include "devices.hpp"
#include "constants.hpp"


class ParserError: public std::exception {
private:
    std::string message;

public:
    ParserError(std::string msg): message(msg) {};
    ParserError(yaml_event_t *event, std::string message);
    ParserError(const ScalarElement &token, std::string message);
    ParserError(const ParserError &a): message(a.message) {};
    ~ParserError() throw() {};
    const char *what() const throw() {
        return message.c_str();
    }
};


class TCPPortStruct: public IntegerStruct {
public:
    void check(BaseUserData*) {
        if(value < PORT_LOWER_BOUND || value > PORT_UPPER_BOUND) {
            std::stringstream buf;
            buf << "Port number must be in " << PORT_LOWER_BOUND << ".." <<
                PORT_UPPER_BOUND << " range";
            throw ParserError(finish, buf.str());
        }
    }
};


class UserData: public BaseUserData {
public:
    std::map<std::string, BaseStruct*> relays_used, adc_used;
    std::set<std::string> drivers_registered;

    ~UserData() throw() {};
};


class DaemonStruct: public ShellStruct {
public:
    StringStruct pidfile;
    StringStruct logfile;

    DaemonStruct() {
        add_required_field("pidfile", &pidfile);
        add_required_field("logfile", &logfile);
    }

    void check(BaseUserData *) {
        if(pidfile.value == "") {
            throw ParserError(pidfile.finish, "Empty pid file name");
        }
        if(logfile.value == "") {
            throw ParserError(logfile.finish, "Empty log file name");
        }
        if (logfile.value == pidfile.value) {
            std::stringstream buf;
            buf << pidfile.start << " and " << logfile.start <<
                " are set to the same path, are your serious?";
            throw ParserError(ScalarElement(), buf.str());
        }
    }
};


class ConnectionStruct: public ShellStruct {
public:
    StringStruct host;
    TCPPortStruct port;
    StringStruct identity;

    ConnectionStruct() {
        add_required_field("host", &host);
        add_required_field("port", &port);
        add_required_field("identity", &identity);
    }
};


class SerialDriverStruct: public ShellStruct {
public:
    StringStruct type;
    StringStruct path;

    SerialDriverStruct() {
        add_required_field("type", &type);
        add_required_field("path", &path);
    }

    void check(BaseUserData *_data) {
        UserData *userdata = dynamic_cast<UserData*>(_data);
        if(type.value != "serial") {
            std::stringstream buf;
            buf << "Unknown driver type, currently only `serial` is supported";
            throw ParserError(type.finish, buf.str());
        }
        if(path.value == "") {
            throw ParserError(type.finish, "Empty serial driver path");
        }

        userdata->drivers_registered.insert(start);
    }
};


class SerialDeviceStruct: public ShellStruct {
public:
    StringStruct type;
    StringStruct relay;
    StringStruct temperature;
    FloatStruct factor;
    FloatStruct shift;

    SerialDeviceStruct() {
        add_required_field("type", &type);
        add_optional_field("relay", &relay);
        add_optional_field("temperature", &temperature);
        add_optional_field("factor", &factor);
        add_optional_field("shift", &shift);
    }

    std::pair<std::string, int> decompose_driver_bind(
        ScalarElement &source, UserData *userdata = NULL) {
        std::list<std::string> dest;
        std::stringstream buf(source);
        std::string item;
        while(getline(buf, item, '.')) {
            dest.push_back(item);
        }
        if(dest.size() != 2) {
            throw ParserError(
                source,
                "Wrong driver bind format, must be <driver name>:<PORT:int>");
        }
        item = dest.front();
        if(userdata != NULL &&
           userdata->drivers_registered.find(item) ==
           userdata->drivers_registered.end()) {
            throw ParserError(
                source, item + ": unknown driver name");
        }
        char *endptr;
        const char *src = dest.back().c_str();
        int val = (int)strtol(src, &endptr, 10);
        if(src == endptr) {
            throw ParserError(
                source,
                dest.back() + ": not a number");
        }
        return std::pair<std::string, int>(dest.front(), val);
    }

    void check(BaseUserData *_data) {
        UserData *userdata = dynamic_cast<UserData*>(_data);
        bool check_relay = false;
        bool check_temperature = false;
        bool check_transform = false;
        if(type.value == "switcher") {
            check_relay = true;
        } else if(type.value == "thermalswitcher") {
            check_temperature = true;
            check_transform = true;
        } else if(type.value == "boiler") {
            check_relay = true;
            check_temperature = true;
            check_transform = true;
        } else {
            throw ParserError(type.finish, "Unsupported device type");
        }

        std::map<std::string, BaseStruct*>::iterator it;
        if(check_relay) {
            it = userdata->relays_used.find(relay.value);
            if(it != userdata->relays_used.end()) {
                std::stringstream buf;
                buf << "Already bound in another device: " << it->second->start;
                throw ParserError(relay.finish, buf.str());
            }
            if(relay.null) {
                throw ParserError(
                    start, "Device must provide relay link");
            }
            std::pair<std::string, int> couple =
                decompose_driver_bind(relay.finish, userdata);
            if(couple.second < RELAY_LOWER_BOUND ||
               couple.second > RELAY_UPPER_BOUND) {
                std::stringstream buf;
                buf << "Port number is out of range, must be within " <<
                    RELAY_LOWER_BOUND << ".." << RELAY_UPPER_BOUND << " range";
                throw ParserError(relay.value, buf.str());
            }
            userdata->relays_used[relay.value] = this;
        }
        if(check_temperature) {
            it = userdata->adc_used.find(temperature.value);
            if(it != userdata->adc_used.end()) {
                std::stringstream buf;
                buf << "Already bound in another device: " << it->second->start;
                throw ParserError(temperature.finish, buf.str());
            }
            if(temperature.null) {
                throw ParserError(
                    start, "Device must provide temperature link");
            }
            std::pair<std::string, int> couple =
                decompose_driver_bind(temperature.finish, userdata);
            if(couple.second < ADC_LOWER_BOUND ||
               couple.second > ADC_UPPER_BOUND) {
                std::stringstream buf;
                buf << "Port number is out of range, must be within " <<
                    ADC_LOWER_BOUND << ".." << ADC_UPPER_BOUND << " range";
                throw ParserError(temperature.value, buf.str());
            }
            userdata->adc_used[temperature.value] = this;
        }
        if(check_transform) {
            if(factor.null) {
                throw ParserError(
                    start, "Device must provide factorization constant");
            }
            if(shift.null) {
                throw ParserError(
                    start, "Device must provide shifting constant");
            }
        }
    }
};


typedef MappingStruct<SerialDriverStruct> DriversStruct;
typedef MappingStruct<SerialDeviceStruct> DevicesStruct;


class ConfigStruct: public ShellStruct {
public:
    DaemonStruct daemon;
    ConnectionStruct connection;
    DriversStruct drivers;
    DevicesStruct devices;

    ConfigStruct() {
        add_required_field("daemon", &daemon);
        add_required_field("connection", &connection);
        add_required_field("drivers", &drivers);
        add_required_field("devices", &devices);
    }

    void check(BaseUserData *data) {
        daemon.check(data);
        connection.check(data);
        drivers.check(data);
        devices.check(data);
    }
};


class config_drivers_t: public std::map<std::string, BaseDriver*> {
public:
    virtual ~config_drivers_t() throw() {
        for(config_drivers_t::iterator it = this->begin();
            it != this->end(); it++) {
            delete it->second;
        }
    }
};


struct config_connection_t {
    std::string host, identity;
    int port;
};


struct config_daemon_t {
    std::string logfile, pidfile;
};


class config_devices_t: public std::map<std::string, BaseDevice*> {
public:
    virtual ~config_devices_t() throw() {
        for(config_devices_t::iterator it=this->begin();
            it != this->end(); it++) {
            delete it->second;
        }
    }

    std::string view() const;
};


class Config {
private:
    config_drivers_t *_drivers;
    config_connection_t *_connection;
    config_daemon_t *_daemon;
    config_devices_t *_devices;

public:
    static int version;
    static time_t startup;
    static time_t conf_change;
    static std::string md5hexdigest;

    Config(config_drivers_t *drvrs,
           config_connection_t *conn,
           config_daemon_t *dmn,
           config_devices_t *dvcs) {
        _drivers = drvrs;
        _connection = conn;
        _daemon = dmn;
        _devices = dvcs;
    }

    virtual ~Config() throw() {
        delete _drivers;
        delete _connection;
        delete _daemon;
        delete _devices;
    }

    const config_drivers_t &drivers() const throw() {
        return *_drivers;
    }

    const config_connection_t &connection() const throw() {
        return *_connection;
    }

    const config_daemon_t &daemon() const throw() {
        return *_daemon;
    }

    const config_devices_t &devices() const throw() {
        return *_devices;
    }

    static Config* get_from_struct(ConfigStruct *data);
};

#endif
