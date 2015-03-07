#include <set>
#include <memory>
#include <sstream>

#include "confparser.hpp"

using namespace std;


ParserError::ParserError(const yaml_event_t &event, string message) {
    stringstream buf;
    buf << "Error at (" << event.start_mark.line + 1 << "," << event.start_mark.column <<
        ")-(" << event.end_mark.line + 1 << "," << event.end_mark.column << "): " << message;

    this->message = buf.str();
}


ParserError::ParserError(const ScalarElement &token, string message) {
    stringstream buf;
    buf << "Error at (" << token.start_pos().first  << "," << token.start_pos().second <<
        ")-(" << token.end_pos().first << "," << token.end_pos().second << "): " <<
        token << " - " << message;

    this->message = buf.str();
}


void token_proceed(yaml_parser_t &parser, yaml_event_t &event) {
    yaml_event_t new_event;
    if(!yaml_parser_parse(&parser, &new_event)) {
        throw ParserError(event, "parsing error after this token");
    }
    event = new_event;
}


void token_type_match(yaml_event_t &event, yaml_event_type_e type_name, std::string message) {
    if(event.type != type_name) {
        throw ParserError(event, message);
    }
}


ParsedElement *parse_all_config(yaml_parser_t &parser) {
    yaml_event_t event;
    ParsedElement *value;
    MapElement *result;

    token_proceed(parser, event);
    switch(event.type) {
    case YAML_SCALAR_EVENT:
        return new ScalarElement(event);
        break;
    case YAML_MAPPING_START_EVENT:
        result = new MapElement;
        break;
    default:
        throw UnsupportedConstructionError(event, "only mappings and strings are supported");
    }

    MapElement &res = *result;
    try {
        while(true) {
            token_proceed(parser, event);
            if(event.type == YAML_MAPPING_END_EVENT) {
                break;
            }
            token_type_match(event, YAML_SCALAR_EVENT, "Section name expected");

            ScalarElement key(event);
            MapElement::iterator it = res.find(key);
            if(it != res.end()) {
                stringstream buf;
                buf << key << ": already defined at (" <<
                    it->first.start_pos().first << "," << it->first.start_pos().second <<
                    ")";
                throw ParserError(key, buf.str());
            }

            res[ScalarElement(event)] = parse_all_config(parser);
        }
    } catch(...) {
        delete result;
        throw;
    }

    return result;
}


MapElement *raw_conf_parse(FILE *fp) {
    yaml_parser_t parser;
    yaml_event_t  event;
    MapElement *result;

    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        fputs("Failed to initialize parser!\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, fp);

    result = NULL;
    do {
        token_proceed(parser, event);

        switch(event.type) {
        case YAML_NO_EVENT: break;
        case YAML_STREAM_START_EVENT: break;
        case YAML_STREAM_END_EVENT: break;

        case YAML_DOCUMENT_START_EVENT:;
            result = dynamic_cast<MapElement*>(parse_all_config(parser));
            if(result == (MapElement*)NULL) {
                throw ParserError(event, "Must be a mapping");
            }
            break;

        case YAML_DOCUMENT_END_EVENT: break;

        case YAML_SEQUENCE_START_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            UnsupportedConstructionError(event, "Sequences are not allowed");
            break;

        case YAML_MAPPING_START_EVENT: break;
        case YAML_MAPPING_END_EVENT: break;
        }

        if(event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }
    } while(event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);

    /* Cleanup */
    yaml_parser_delete(&parser);
    return result;

}


template <class T> class FieldGetter {
private:
    string proper_type, wrong_type;
    ScalarElement _parent_field;

public:
    FieldGetter(string proper, string wrong): proper_type(proper), wrong_type(wrong) {};

    void raise_exception(ScalarElement &parent, string message) {
        if(parent.length() > 0) {
            throw ParserError(parent, message);
        } else {
            throw ParserError(message);
        }
    }

    string no_key_exception(ScalarElement parent, string key)  {
        stringstream buf;
        string section_name = (parent.length() > 0)?parent:"root";

        buf << "no required field " << key << " found in " << section_name << " section";
        raise_exception(parent, buf.str());
    }

    string wrong_type_exception(ScalarElement key) {
        stringstream buf;
        buf << "field " << key << " must define a " << proper_type << " not " << wrong_type;
        raise_exception(key, buf.str());
    }

    T &get_field(MapElement &mapping, ScalarElement parent, string key) {
        MapElement::iterator it = mapping.find(key);
        if(it == mapping.end()) {
            no_key_exception(parent, key);
        }

        T *value = dynamic_cast<T*>(it->second);
        if(value == (T*)NULL) {
            wrong_type_exception(it->first);
        }
        _parent_field = it->first;
        return *value;
    }

    void check_fields(MapElement &mapping, ScalarElement parent, string allowed_fields) {
        istringstream buf(allowed_fields);
        string token;
        set<string> allowed;
        while(getline(buf, token, ',')) {
            allowed.insert(token);
        }

        for(MapElement::iterator it = mapping.begin(); it != mapping.end(); it++) {
            if(allowed.count(it->first) == 0) {
                stringstream buf;
                buf << "field " << it->first << " is not supported in section " << parent;
                throw ParserError(it->first, buf.str());
            }
        }
    }

    ScalarElement parent_name() throw() {
        return _parent_field;
    }
};


class ScalarGetter: public FieldGetter<ScalarElement> {
public:
    ScalarGetter(std::string proper, std::string wrong): FieldGetter<ScalarElement>(proper, wrong) {};

    int get_number(MapElement& mapping, ScalarElement parent, string key, int lower_bound, int upper_bound) {
        if(lower_bound > upper_bound) {
            throw ParserError(parent, "lower bound must not be greater than upper");
        }
        ScalarElement data = get_field(mapping, parent, key);
        char *p;
        long converted = strtol(data.c_str(), &p, 10);
        bool fail_condition = *p;
        if (lower_bound < upper_bound) {
            fail_condition = *p || converted < lower_bound || converted > upper_bound;
        }
        if(fail_condition) {
            stringstream buf;
            buf << parent_name() << " must be a number";
            if(lower_bound < upper_bound) {
                buf << " in " << lower_bound << ".." << upper_bound << " range";
            }
            throw ParserError(data, buf.str());
        }
        return (int)converted;

    }

    ScalarElement get_string(MapElement& mapping, ScalarElement parent, string key, bool not_empty=false) {
        ScalarElement data = get_field(mapping, parent, key);
        if(data.length() < 1 && not_empty) {
            throw ParserError(data, parent_name() + " must not be empty");
        }
        return data;
    }
};


FieldGetter<MapElement> map_getter("mapping", "scalar");
ScalarGetter scalar_getter("scalar", "mapping");


Config *config_parse(MapElement *_rawconf) {
    Config *cfg;
    MapElement &rawconf = *_rawconf;

    map_getter.check_fields(rawconf, "root", "connection,daemon,drivers");
    // Form connection
    MapElement &connection = map_getter.get_field(rawconf, "", "connection");
    ScalarElement conn_parent = map_getter.parent_name();
    map_getter.check_fields(connection, conn_parent, "host,port,identity");
   auto_ptr<config_connection_t> conn(new config_connection_t);
    conn->host = scalar_getter.get_string(connection, conn_parent, "host", true);
    conn->port = scalar_getter.get_number(connection, conn_parent, "port", PORT_LOWER_BOUND, PORT_UPPER_BOUND);
    conn->identity = scalar_getter.get_string(connection, conn_parent, "identity", true);

    // Form daemon data
    MapElement &daemon = map_getter.get_field(rawconf, "", "daemon");
    ScalarElement daemon_parent = map_getter.parent_name();
    map_getter.check_fields(daemon, daemon_parent, "logfile,pidfile");
    auto_ptr<config_daemon_t> dmn(new config_daemon_t);
    dmn->logfile = scalar_getter.get_field(daemon, daemon_parent, "logfile");
    dmn->pidfile = scalar_getter.get_field(daemon, daemon_parent, "pidfile");

    // Form drivers
    MapElement &drivers = map_getter.get_field(rawconf, "", "drivers");
    ScalarElement drivers_parent = map_getter.parent_name();
    auto_ptr<config_drivers_t> drvrs(new config_drivers_t);
    map<ScalarElement, ScalarElement> serial_device_path;
    for(MapElement::iterator it = drivers.begin(); it != drivers.end(); it++) {
        MapElement &driverconf = map_getter.get_field(drivers, drivers_parent, it->first);
        ScalarElement driver_parent = map_getter.parent_name();
        map_getter.check_fields(driverconf, driver_parent, "path,type");

        ScalarElement device_type = scalar_getter.get_string(driverconf, driver_parent, "type", true);
        if(device_type == SERIAL_DESCRIPTOR) {
            ScalarElement device_path = scalar_getter.get_string(driverconf, driver_parent, "path", true);
            map<ScalarElement, ScalarElement>::iterator duplicate = serial_device_path.find(device_path);
            if(duplicate != serial_device_path.end()) {
                stringstream buf;
                buf << "Serial device path " << device_path << " is also bound at (" <<
                    duplicate->first.start_pos().first << "," << duplicate->first.start_pos().second <<
                    "), section " << duplicate->second;
                throw ParserError(device_path, buf.str());
            }
            serial_device_path[device_path] = driver_parent;
            (*drvrs)[driver_parent] = new SerialDriver(device_path);
        } else {
            throw ParserError(device_type, device_type + " not supported");
        }
    }

    return new Config(drvrs.release(), conn.release(), dmn.release());
}
