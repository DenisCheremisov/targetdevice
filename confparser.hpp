#ifndef _CONFPARSER_H_INCLUDED_
#define _CONFPARSER_H_INCLUDED_

#include <cstdio>
#include <map>
#include <string>
#include <fstream>
#include <iostream>

#include <yaml.h>


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

    location_t start_pos() {
        return _start_pos;
    }

    location_t end_pos() {
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
    ParserError(yaml_event_t &event, std::string message);
    ParserError(ScalarElement &token, std::string message);
    ~ParserError() throw() {};
    const char *what() const throw() {
        return this->message.c_str();
    }
};


class UnsupportedConstructionError: public ParserError {
public:
    UnsupportedConstructionError(yaml_event_t &event, std::string message):
        ParserError(event, message) {};
};


MapElement *raw_conf_parse(FILE *fp);
#endif
