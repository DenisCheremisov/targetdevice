#ifndef _YAMLPARSER_HPP_INCLUDED_
#define _YAMLPARSER_HPP_INCLUDED_

#include <cstdio>

#include <set>
#include <map>
#include <list>
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <exception>

#include <yaml.h>


class YamlBaseError: public std::exception, public std::string {
public:
    YamlBaseError() {}
    YamlBaseError(const char *message): std::string(message) {}
    ~YamlBaseError() throw() {}

    const char *what() const throw() {
        return this->c_str();
    }
};


typedef std::pair<size_t, size_t> location_t;
class ScalarElement: public std::string {
    location_t _start_pos, _end_pos;

public:
    ScalarElement(yaml_event_t *event):
        std::string((char*)event->data.scalar.value) {
        _start_pos = location_t(event->start_mark.line + 1,
                                event->start_mark.column);
        _end_pos = location_t(event->end_mark.line + 1,
                              event->end_mark.column);
    }
    ScalarElement(std::string &token): std::string(token) {};
    ScalarElement(const char *data): std::string(data) {};
    ScalarElement(const ScalarElement &source):
        std::string(source),
        _start_pos(source._start_pos),
        _end_pos(source._end_pos) {};

    ScalarElement() {};
    ~ScalarElement() throw() {};

    const location_t start_pos() const {
        return _start_pos;
    }

    const location_t end_pos() const {
        return _end_pos;
    }

    bool operator<(const ScalarElement &another) {
        return (
            _start_pos.first < another._start_pos.first ||
            _start_pos.second < another._start_pos.second);
    }

};


class YamlEvent: public yaml_event_t {
    YamlEvent() {};

public:
    ~YamlEvent() throw() {
        yaml_event_delete(this);
    }

    friend class YamlParser;
};


std::ostream& operator<<(std::ostream& buf, const ScalarElement& data) {
    buf << (std::string)data << " (" << data.start_pos().first << "," <<
        data.start_pos().second << " - " <<
        data.end_pos().first << "," << data.end_pos().second << ")";
    return buf;
}


class YamlParserError: public YamlBaseError {
public:
    YamlParserError(YamlEvent *event, const char *message) {
        std::stringstream buf;
        buf << "Error at (" << event->start_mark.line + 1 << "," <<
            event->start_mark.column << ")-(" << event->end_mark.line + 1 <<
            "," << event->end_mark.column << "): " << message;
        delete event;
        assign(buf.str());
    }

    YamlParserError(const ScalarElement &token, std::string message) {
        std::stringstream buf;
        buf << "Error at (" << token.start_pos().first  << "," <<
            token.start_pos().second << ")-(" << token.end_pos().first <<
            "," << token.end_pos().second << "): " <<
            token << " - " << message;

        this->assign(buf.str());
    }
};


class YamlStructureError: public YamlParserError {
public:
    YamlStructureError(YamlEvent *event, const char *message):
        YamlParserError(event, message) {}

    YamlStructureError(const ScalarElement &token, std::string message):
        YamlParserError(token, message) {}
};


class YamlParser {
private:
    bool master;
    yaml_parser_t *parser;
    bool just_started;
    std::list<YamlEvent*> *event_list;
    std::list<YamlEvent*>::iterator event_it;

    YamlParser(): master(true) {
        parser = new yaml_parser_t;
        event_list = new std::list<YamlEvent*>;
        event_it = event_list->begin();
        just_started = true;

        if(!yaml_parser_initialize(parser)) {
            throw YamlBaseError("Cannot initialize parser");
        }
    }

public:
    YamlParser(const YamlParser &prsr) {
        master = false;
        parser = prsr.parser;
        just_started = prsr.just_started;
        event_list = prsr.event_list;
        event_it = prsr.event_it;
    }

    void confirm(const YamlParser &prsr) {
        event_it = prsr.event_it;
    }

    static YamlParser *get(FILE *fp) {
        YamlParser *res = new YamlParser;
        yaml_parser_set_input_file(res->parser, fp);
        return res;
    }

    static YamlParser *get(const unsigned char *input, size_t size) {
        YamlParser *res = new YamlParser;
        yaml_parser_set_input_string(res->parser, input, size);
        return res;
    }

    static YamlParser *get(const std::string &source) {
        return get(reinterpret_cast<const unsigned char*>(source.c_str()),
                   source.size());
    }

    virtual ~YamlParser() throw() {
        if(master) {
            yaml_parser_delete(parser);
            delete parser;

            for(std::list<YamlEvent*>::iterator it = event_list->begin();
                it != event_list->end(); it++) {
                delete *it;
            }
            delete event_list;
        }
    }

    YamlEvent *get_event() {
        if(++event_it == event_list->end()) {
            YamlEvent *event = new YamlEvent;
            if(!yaml_parser_parse(parser, event)) {
                throw YamlParserError(event, "parsing error after this token");
            }
            event_list->push_back(event);
            event_it = event_list->end();
            --event_it;
        }
        if(just_started) {
            event_it = event_list->begin();
            just_started = false;
        }
        return *(event_it);
    }
};


class BaseStruct {
public:
    ScalarElement start, finish;

    BaseStruct() {}
    virtual ~BaseStruct() throw() {}

    virtual BaseStruct *build(YamlParser *parser) = 0;
    void set_start(const ScalarElement &st) {
        start = st;
    }
    void set_finish(const ScalarElement &fin) {
        finish = fin;
    }
};


class IntegerStruct: public BaseStruct {
public:
    bool null;
    int value;

    IntegerStruct(): null(true), value(0) {
    }
    ~IntegerStruct() throw() {};

    BaseStruct *build(YamlParser *parser) {
        YamlEvent *event = parser->get_event();
        bool fail = false;
        if(event->type != YAML_SCALAR_EVENT) {
            fail = true;
        }
        if(!fail) {
            ScalarElement data(event);
            char *p;
            long converted = strtol(data.c_str(), &p, 10);
            fail = *p;
            if(!fail) {
                set_finish(data);
                null = false;
                value = (int)converted;
            }
        }
        if(fail) {
            std::stringstream buf;
            buf << "Wrong format for " << start <<
                " - must be an integer number";
            throw YamlStructureError(event, buf.str());
        }
        return this;
    }
};


class FloatStruct: public BaseStruct {
public:
    bool null;
    double value;

    FloatStruct(): null(true), value(0) {
    }
    ~FloatStruct() throw() {};

    BaseStruct *build(YamlParser *parser) {
        YamlEvent *event = parser->get_event();
        bool fail = false;
        if(event->type != YAML_SCALAR_EVENT) {
            fail = true;
        }
        if(!fail) {
            ScalarElement data(event);
            char *p;
            double converted = strtod(data.c_str(), &p);
            fail = *p;
            if(!fail) {
                set_finish(data);
                null = false;
                value = converted;
            }
        }
        if(fail) {
            std::stringstream buf;
            buf << "Wrong format for " << start <<
                " - must be a floating number";
            throw YamlStructureError(event, buf.str());
        }
        return this;
    }
};


class StringStruct: public BaseStruct {
public:
    bool null;
    std::string value;

    StringStruct(): null(true), value("") {
    }
    ~StringStruct() throw() {};

    BaseStruct *build(YamlParser *parser) {
        YamlEvent *event = parser->get_event();
        if(event->type != YAML_SCALAR_EVENT) {
            std::stringstream buf;
            buf << "Wrong format for " << start << " - must be a text";
            throw YamlStructureError(event, buf.str());
        }
        ScalarElement data(event);
        set_finish(data);
        null = false;
        value.assign(data);
        return this;
    }
};


typedef std::map<std::string, BaseStruct*> str2struct;
class ShellStruct: public BaseStruct {
private:
    str2struct required;
    str2struct optional;

public:
    bool null;

    ShellStruct(): null(true) {}
    ~ShellStruct() throw() {};

    void add_required_field(std::string field_name, BaseStruct *structure) {
        required[field_name] = structure;
    }

    void add_optional_field(std::string field_name, BaseStruct *structure) {
        optional[field_name] = structure;
    }

    BaseStruct *build(YamlParser *parser) {
        YamlEvent *event = parser->get_event();
        if(event->type != YAML_MAPPING_START_EVENT) {
            std::stringstream buf;
            buf << "Wrong node for " << start << " - must a mapping";
            throw YamlStructureError(event, buf.str());
        }

        std::set<std::string> required_elements;
        std::set<ScalarElement> used_elements;
        for(str2struct::iterator it = required.begin();
            it != required.end(); it++) {
            required_elements.insert(it->first);
        }

        str2struct::iterator req, opt;
        std::set<ScalarElement>::iterator used;
        while(true) {
            YamlEvent *event = parser->get_event();
            if(event->type == YAML_MAPPING_END_EVENT) {
                break;
            }
            ScalarElement header(event);
            BaseStruct *strct = NULL;
            used = used_elements.find(header);
            if(used != used_elements.end()) {
                std::stringstream buf;
                buf << "This element has been defined already " << *used;
                throw YamlStructureError(header, buf.str());
            }
            req = required.find(header);
            if(req != required.end()) {
                strct = req->second;
                required_elements.erase(header);
            } else {
                opt = optional.find(header);
                if(opt != optional.end()) {
                    strct = opt->second;
                }
            }
            if(strct == NULL) {
                std::stringstream buf;
                throw YamlStructureError(header, "No such field in schema");
            }
            strct->build(parser);
            strct->set_start(header);
        }

        if(required_elements.size() > 0) {
            std::stringstream buf;
            int i = 0;
            std::string name, verb;
            if(required_elements.size() == 1) {
                name = "field";
                verb = "is";
            } else {
                name = "fields";
                verb = "are";
            }
            buf << "Required " << name << " ";
            for(std::set<std::string>::iterator it = required_elements.begin();
                it != required_elements.end(); it++) {
                if(i > 0) {
                    buf << ", ";

                } else {
                    i++;
                }
                buf << "`" << *it << "`";
            }
            buf << " " << verb << " not defined";
            throw YamlBaseError(buf.str().c_str());
        }
        return this;
    }
};


template <class F, class S>
class ChoiceStruct: public BaseStruct {
public:
    BaseStruct *build(YamlParser *parser) {
        std::auto_ptr<F> res(new F);
        YamlParser prsr(*parser);
        try {
            BaseStruct *r = res->build(&prsr);
            if(r == res.get()) {
                res.release();
            }
            parser->confirm(prsr);
            return r;
        } catch(YamlBaseError e) {
            std::auto_ptr<S> res(new S);
            BaseStruct *r = res->build(parser);
            if(r == res.get()) {
                res.release();
            }
            return r;
        }
    }
};


template <class T>
class MappingStruct: public BaseStruct,
                     public std::map<ScalarElement, BaseStruct*> {
public:
    MappingStruct() {
    }
    ~MappingStruct() throw() {
        for(std::map<ScalarElement, BaseStruct*>::iterator it = this->begin();
            it != this->end(); it++) {
            delete it->second;
        }
    }

    BaseStruct *build(YamlParser *parser) {
        YamlEvent *event = parser->get_event();
        if(event->type != YAML_MAPPING_START_EVENT) {
            throw YamlStructureError(event, "Must be a mapping here");
        }
        while(event = parser->get_event(),
              event->type != YAML_MAPPING_END_EVENT) {
            std::auto_ptr<T> res(new T);
            BaseStruct *r = res->build(parser);
            if(r == res.get()) {
                res.release();
            }
            ScalarElement header(event);
            r->set_start(header);
            (*this)[header] = r;
        }
        return this;
    }
};


BaseStruct *yaml_parse(YamlParser *parser, BaseStruct *strct) {
    BaseStruct *res;
    while(true) {
        YamlEvent *event = parser->get_event();
        switch(event->type) {
        case YAML_NO_EVENT: break;
        case YAML_STREAM_START_EVENT: break;
        case YAML_STREAM_END_EVENT: break;

        case YAML_DOCUMENT_START_EVENT:;
            res = strct->build(parser);
            break;

        case YAML_DOCUMENT_END_EVENT: break;

        case YAML_SEQUENCE_START_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            YamlParserError(event, "Sequences are not allowed");
            break;

        case YAML_MAPPING_START_EVENT: break;
        case YAML_MAPPING_END_EVENT: break;
        default: ;
        }
        if(event->type == YAML_DOCUMENT_END_EVENT) {
            break;
        }
    }
    return res;
}


#endif // _YAMLPARSER_HPP_INCLUDED_
