#include <set>
#include <list>
#include <memory>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "confparser.hpp"

using namespace std;

int Config::version = 1;
time_t Config::startup = time(NULL);
time_t Config::conf_change = 0;

ParserError::ParserError(yaml_event_t *event, string message) {
    stringstream buf;
    buf << "Error at (" << event->start_mark.line + 1 << "," << event->start_mark.column <<
        ")-(" << event->end_mark.line + 1 << "," << event->end_mark.column << "): " << message;

    this->message = buf.str();
}


ParserError::ParserError(const ScalarElement &token, string message) {
    stringstream buf;
    buf << "Error at (" << token.start_pos().first  << "," << token.start_pos().second <<
        ")-(" << token.end_pos().first << "," << token.end_pos().second << "): " <<
        token << " - " << message;

    this->message = buf.str();
}


class Parser: public yaml_parser_t {
private:
    bool initialized;

public:
    Parser(): initialized(false) {};
    ~Parser() throw() {
        if(initialized) {
            yaml_parser_delete(this);
        }
    }
    bool initialize() {
        initialized = (bool)yaml_parser_initialize(this);
        return initialized;
    }
};


class Event: public yaml_event_t {
private:
    bool initialized;

public:
    Event(): initialized(false) {};
    ~Event() throw() {
        if(initialized) {
            yaml_event_delete(this);
        }
    }
    void proceed(Parser &parser) {
        if(!yaml_parser_parse(&parser, this)) {
            throw ParserError(this, "parsing error after this token");
        }
        initialized = true;
    }
    void match(yaml_event_type_e type_name, std::string message) {
        if(type != type_name) {
            throw ParserError(this, message);
        }
    }
    void free() throw() {
        if(initialized) {
            yaml_event_delete(this);
            initialized = false;
        }
    }
};


ParsedElement *parse_all_config(Parser &parser) {
    Event event;
    ParsedElement *value;
    MapElement *result;

    event.proceed(parser);
    switch(event.type) {
    case YAML_SCALAR_EVENT:
        return new ScalarElement(static_cast<yaml_event_t*>(&event));
        break;
    case YAML_MAPPING_START_EVENT:
        result = new MapElement;
        break;
    default:
        throw UnsupportedConstructionError(&event, "only mappings and strings are supported");
    }

    MapElement &res = *result;
    try {
        while(true) {
            event.proceed(parser);
            if(event.type == YAML_MAPPING_END_EVENT) {
                break;
            }
            event.match(YAML_SCALAR_EVENT, "Section name expected");

            ScalarElement key(static_cast<yaml_event_t*>(&event));
            MapElement::iterator it = res.find(key);
            if(it != res.end()) {
                stringstream buf;
                buf << key << ": also defined at (" <<
                    it->first.start_pos().first << "," << it->first.start_pos().second <<
                    ")";
                throw ParserError(key, buf.str());
            }

            res[ScalarElement(static_cast<yaml_event_t*>(&event))] = parse_all_config(parser);
            event.free();
        }
    } catch(...) {
        delete result;
        throw;
    }

    return result;
}


MapElement *raw_conf_parse(FILE *fp) {
    Parser parser;
    Event event;
    MapElement *result;

    /* Initialize parser */
    if(!parser.initialize()) {
        fputs("Failed to initialize parser!\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, fp);

    result = NULL;
    do {
        event.proceed(parser);

        switch(event.type) {
        case YAML_NO_EVENT: break;
        case YAML_STREAM_START_EVENT: break;
        case YAML_STREAM_END_EVENT: break;

        case YAML_DOCUMENT_START_EVENT:;
            result = dynamic_cast<MapElement*>(parse_all_config(parser));
            if(result == (MapElement*)NULL) {
                throw ParserError(&event, "Must be a mapping");
            }
            break;

        case YAML_DOCUMENT_END_EVENT: break;

        case YAML_SEQUENCE_START_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            UnsupportedConstructionError(&event, "Sequences are not allowed");
            break;

        case YAML_MAPPING_START_EVENT: break;
        case YAML_MAPPING_END_EVENT: break;
        default: ;
        }

        if(event.type != YAML_STREAM_END_EVENT) {
            event.free();
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

    void no_key_exception(ScalarElement parent, string key)  {
        stringstream buf;
        string section_name = (parent.length() > 0)?parent:"root";

        buf << "no required field " << key << " found in " << section_name << " section";
        raise_exception(parent, buf.str());
    }

    string wrong_type_exception(ScalarElement key) {
        stringstream buf;
        buf << "field " << key << " must define a " << proper_type << " not " << wrong_type;
        raise_exception(key, buf.str());
        return buf.str();
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

    void check_fields(MapElement &mapping, ScalarElement parent, string allowed_fields, string description = "") {
        istringstream buf(allowed_fields);
        string token;
        set<string> allowed;
        while(getline(buf, token, ',')) {
            allowed.insert(token);
        }

        for(MapElement::iterator it = mapping.begin(); it != mapping.end(); it++) {
            if(allowed.count(it->first) == 0) {
                stringstream buf;
                buf << "field " << it->first  << " is not supported in ";
                if(description.length() > 0) {
                    buf << description << " " << parent;
                } else {
                    buf << "section " << parent;
                }
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

    int extract_number(string data, int lower_bound, int upper_bound) {
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

    double extract_float(string data) {
        char *p;
        double converted = strtod(data.c_str(), &p);
        bool fail_condition = *p;
        if(fail_condition) {
            stringstream buf;
            buf << parent_name() << " must be a real number";
            throw ParserError(data, buf.str());
        }
        return converted;
    }

    int get_number(MapElement& mapping, ScalarElement parent, string key, int lower_bound, int upper_bound) {
        if(lower_bound > upper_bound) {
            throw ParserError(parent, "lower bound must not be greater than upper");
        }
        ScalarElement data = get_field(mapping, parent, key);
        return extract_number(data, lower_bound, upper_bound);
    }

    int get_double(MapElement &mapping, ScalarElement parent, string key) {
        ScalarElement data = get_field(mapping, parent, key);
        return extract_float(data);
    }

    ScalarElement get_string(MapElement& mapping, ScalarElement parent, string key, bool not_empty=false) {
        ScalarElement data = get_field(mapping, parent, key);
        if(data.length() < 1 && not_empty) {
            throw ParserError(data, parent_name() + " must not be empty");
        }
        return data;
    }

    serial_link_t get_link(MapElement& mapping, ScalarElement parent, string key,
                           auto_ptr<config_drivers_t> &drivers,
                           map<ScalarElement, ScalarElement> &used_paths, int lower_bound, int upper_bound) {
        ScalarElement data = get_string(mapping, parent, key, true);
        map<ScalarElement, ScalarElement>::iterator it = used_paths.find(data);
        if(it != used_paths.end()) {
            stringstream buf;
            buf << "port " << data << " has been bound already in " << it->second << " at (" <<
                it->first.start_pos().first << "," << it->first.start_pos().second << ")";
            throw ParserError(data, buf.str());
        }
        istringstream split_buf(data);
        string driver_name;
        if(!getline(split_buf, driver_name, '.')) {
            throw ParserError(data, "wrong format, must be <driver name>.<port> notation");
        }
        if(drivers->count(driver_name) < 1) {
            throw ParserError(data, data + " no such driver defined");
        }
        string port_data;
        if(!getline(split_buf, port_data, '.')) {
            throw ParserError(data, "wrong format, must be <driver name>.<port> notation");
        }
        used_paths[data] = parent;
        int port = extract_number(port_data, lower_bound, upper_bound);
        return serial_link_t(driver_name, port);
    }
};


bool compare_scalars(const ScalarElement &o1, const ScalarElement &o2) {
    return o1 < o2 || o1.start_pos() < o2.start_pos();
}


FieldGetter<MapElement> map_getter("mapping", "scalar");
ScalarGetter scalar_getter("scalar", "mapping");


Config *config_parse(MapElement *_rawconf) {
    Config *cfg;
    MapElement &rawconf = *_rawconf;

    map_getter.check_fields(rawconf, "root", "connection,daemon,drivers,devices");
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

    // For duplication detection needs
    list<ScalarElement> key_list;

    // Form drivers
    MapElement &drivers = map_getter.get_field(rawconf, "", "drivers");
    ScalarElement drivers_parent = map_getter.parent_name();
    auto_ptr<config_drivers_t> drvrs(new config_drivers_t);
    map<ScalarElement, ScalarElement> serial_device_path;
    key_list.clear();
    for(MapElement::iterator it = drivers.begin(); it != drivers.end(); it++) {
        key_list.push_back(it->first);
    }
    key_list.sort(compare_scalars);
    for(list<ScalarElement>::iterator it = key_list.begin(); it != key_list.end(); it++) {
        MapElement &driverconf = map_getter.get_field(drivers, drivers_parent, *it);
        ScalarElement driver_parent = map_getter.parent_name();

        ScalarElement device_type = scalar_getter.get_string(driverconf, driver_parent, "type", true);
        if(device_type == SERIAL_DESCRIPTOR) {
            map_getter.check_fields(driverconf, driver_parent, "type,path", "serial device description");
            ScalarElement device_path = scalar_getter.get_string(driverconf, driver_parent, "path", true);
            map<ScalarElement, ScalarElement>::iterator duplicate = serial_device_path.find(device_path);
            if(duplicate != serial_device_path.end()) {
                stringstream buf;
                buf << driver_parent << " - serial device path " << device_path << " has been bound already at (" <<
                    duplicate->first.start_pos().first << "," << duplicate->first.start_pos().second <<
                    ") to device " << duplicate->second;
                throw ParserError(device_path, buf.str());
            }
            serial_device_path[device_path] = driver_parent;
            (*drvrs)[driver_parent] = new SerialDriver(device_path);
        } else {
            throw ParserError(device_type, device_type + " not supported");
        }
    }

    // Form devices
    MapElement &devices = map_getter.get_field(rawconf, "", "devices");
    ScalarElement devices_parent = map_getter.parent_name();
    auto_ptr<config_devices_t> dvcs(new config_devices_t);
    key_list.clear();
    for(MapElement::iterator it = devices.begin();
        it != devices.end(); it++) {
        key_list.push_back(it->first);
    }
    key_list.sort(compare_scalars);
    map<ScalarElement, ScalarElement> relay_used, adc_used;
    for(list<ScalarElement>::iterator it = key_list.begin();
        it != key_list.end(); it++) {
        MapElement &deviceconf = map_getter.get_field(
            devices, devices_parent, *it);
        ScalarElement device_parent = map_getter.parent_name();

        ScalarElement device_type = scalar_getter.get_string(
            deviceconf, device_parent, "type", true);
        if(device_type == BOILER_DESCRIPTOR) {
            map_getter.check_fields(deviceconf, device_parent,
                "type,relay,temperature,factor,shift", "boiling device description");
            serial_link_t relay = scalar_getter.get_link(
                deviceconf, device_parent, "relay", drvrs, relay_used,
                RELAY_LOWER_BOUND, RELAY_UPPER_BOUND);
            serial_link_t temperature = scalar_getter.get_link(
                deviceconf, device_parent, "temperature", drvrs, adc_used,
                ADC_LOWER_BOUND, ADC_UPPER_BOUND);
            double factor = scalar_getter.get_double(
                deviceconf, device_parent, "factor");
            double shift = scalar_getter.get_double(
                deviceconf, device_parent, "shift");
            (*dvcs)[device_parent] = new Boiler(relay, temperature, factor, shift);
        } else if(device_type == SWITCHER_DESCRIPTOR) {
            map_getter.check_fields(
                deviceconf, device_parent, "type,relay",
                " switcher description");
            serial_link_t relay = scalar_getter.get_link(
                deviceconf, device_parent, "relay", drvrs, relay_used,
                RELAY_LOWER_BOUND, RELAY_UPPER_BOUND);
            (*dvcs)[device_parent] = new Switcher(relay);
        } else if(device_type == THERMALSWITCHER_DESCRIPTOR) {
            map_getter.check_fields(
                deviceconf, device_parent, "type,relay,temperature,factor,shift",
                "temperature measurement device description");
            serial_link_t relay = scalar_getter.get_link(
                deviceconf, device_parent, "relay", drvrs, relay_used,
                RELAY_LOWER_BOUND, RELAY_UPPER_BOUND);
            serial_link_t temperature = scalar_getter.get_link(
                deviceconf, device_parent, "temperature", drvrs,  adc_used,
                ADC_LOWER_BOUND, ADC_UPPER_BOUND);
            double factor = scalar_getter.get_double(
                deviceconf, device_parent, "factor");
            double shift = scalar_getter.get_double(
                deviceconf, device_parent, "shift");
            (*dvcs)[device_parent] = new Thermoswitcher(temperature, factor, shift);
        } else {
            throw ParserError(device_type, device_type + " not supported");
        }
    }

    return new Config(
        drvrs.release(), conn.release(), dmn.release(), dvcs.release());
}


FILE *open_conf_fp(const char *name) {
    FILE *res = fopen(name, "r");
    if(res != NULL) {
        struct stat attrib;
        fstat(res->_fileno, &attrib);
        Config::conf_change = attrib.st_mtim.tv_sec;
    }
    return res;
}
