#ifndef _CONFPARSER_H_INCLUDED_
#define _CONFPARSER_H_INCLUDED_

#include <cstdio>
#include <map>
#include <string>
#include <fstream>
#include <iostream>

#include <yaml.h>


#include "drivers.hpp"
#include "constants.hpp"


typedef std::pair<size_t, size_t> location_t;

class ParsedElement {
public:
    virtual ~ParsedElement() throw() {};
};


class ScalarElement: public ParsedElement, public std::string {
    location_t _start_pos, _end_pos;
    std::string value;

public:
    ScalarElement(yaml_event_t &event): std::string((char*)event.data.scalar.value) {
        _start_pos = location_t(event.start_mark.line + 1,
                              event.start_mark.column);
        _end_pos = location_t(event.end_mark.line + 1,
                             event.end_mark.column);
    }
    ScalarElement(std::string &token): std::string(token) {};
    ScalarElement(const char *data): std::string(data) {};
    ~ScalarElement() throw() {};

    const location_t start_pos() const {
        return _start_pos;
    }

    const location_t end_pos() const {
        return _end_pos;
    }
};


typedef std::map<ScalarElement, ParsedElement*> subtree_t;


class MapElement: public ParsedElement, public subtree_t {
public:
    ~MapElement() throw() {
        for(subtree_t::iterator it = this->begin(); it != this->end(); it++) {
            if(it->second != NULL) {
                delete it->second;
            }
        }
    }
};


class ParserError: public std::exception {
private:
    std::string message;

public:
    ParserError(std::string msg): message(msg) {};
    ParserError(const yaml_event_t &event, std::string message);
    ParserError(const ScalarElement &token, std::string message);
    ~ParserError() throw() {};
    const char *what() const throw() {
        return this->message.c_str();
    }
};


class UnsupportedConstructionError: public ParserError {
public:
    UnsupportedConstructionError(const yaml_event_t &event, std::string message):
        ParserError(event, message) {};
};



class config_drivers_t: public std::map<std::string, BaseDriver*> {
public:
    virtual ~config_drivers_t() {
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


class Config {
private:
    config_drivers_t *_drivers;
    config_connection_t *_connection;
    config_daemon_t *_daemon;

public:
    Config(config_drivers_t *drvrs,
           config_connection_t *conn,
           config_daemon_t *dmn) {
        _drivers = drvrs;
        _connection = conn;
        _daemon = dmn;
    }

    virtual ~Config() throw() {
        delete _drivers;
        delete _connection;
        delete _daemon;
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
};


MapElement *raw_conf_parse(FILE *fp);
Config *config_parse(MapElement *rawconf);
#endif
