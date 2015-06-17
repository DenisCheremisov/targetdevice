#include "confparser.hpp"

using namespace std;

int Config::version = 1;
time_t Config::startup = time(NULL);
time_t Config::conf_change = 0;
string Config::md5hexdigest;

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


string config_devices_t::view() const {
    stringstream buf;
    for(auto &it: *this) {
        buf << it.first << ":";
        switch(it.second->id()) {
        case DEVICE_BOILER:
            buf << "boiler";
            break;
        case DEVICE_SWITCHER:
            buf << "switcher";
            break;
        case DEVICE_THERMOSWITCHER:
            buf << "temperature";
            break;
        default:
            buf << "undefined";
        }
        buf << "\n";
    }
    return buf.str();
}


Config* Config::get_from_struct(ConfigStruct *data) {
    unique_ptr<config_daemon_t> daemon(new config_daemon_t);
    daemon->logfile = data->daemon.logfile.value;
    daemon->pidfile = data->daemon.pidfile.value;

    unique_ptr<config_connection_t> connection(new config_connection_t);
    connection->host = data->connection.host.value;
    connection->port = data->connection.port.value;
    connection->identity = data->connection.identity.value;

    unique_ptr<config_drivers_t> drivers(new config_drivers_t);
    for(DriversStruct::iterator it = data->drivers.begin();
        it != data->drivers.end(); it++) {
        if(dynamic_cast<SerialDriverStruct*>(it->second) != NULL) {
            SerialDriverStruct *driver =
                dynamic_cast<SerialDriverStruct*>(it->second);
            if(driver->type.value == "serial") {
                (*drivers)[it->first] = new SerialDriver(driver->path.value);
            } else {
                throw ParserError(
                    driver->type.finish, "Unsupported driver type");
            }
        } else {
            throw ParserError(
                it->second->start, "Something wrong with this driver");
        }
    }

    unique_ptr<config_devices_t> devices(new config_devices_t);
    for(DevicesStruct::iterator it = data->devices.begin();
        it != data->devices.end(); it++) {
        if(dynamic_cast<SerialDeviceStruct*>(it->second) != NULL) {
            SerialDeviceStruct *device =
                dynamic_cast<SerialDeviceStruct*>(it->second);
            if(device->type.value == "switcher") {
                (*devices)[it->first] = new Switcher(
                    serial_link_t(
                        device->decompose_driver_bind(
                            device->relay.finish, NULL)));
            } else if(device->type.value == "thermalswitcher") {
                (*devices)[it->first] = new Thermoswitcher(
                    serial_link_t(
                        device->decompose_driver_bind(
                            device->temperature.finish, NULL)),
                    device->factor.value, device->shift.value);
            } else if(device->type.value == "boiler") {
                (*devices)[it->first] = new Boiler(
                    serial_link_t(
                        device->decompose_driver_bind(
                            device->relay.finish, NULL)),
                    serial_link_t(
                        device->decompose_driver_bind(
                            device->temperature.finish, NULL)),
                    device->factor.value, device->shift.value);
            } else {
                throw ParserError(
                    device->type.finish, "Unsupported device type");
            }
        } else {
            throw ParserError(
                it->second->start, "Something wrong with this device");
        }
    }

    return new Config(
        drivers.release(), connection.release(),
        daemon.release(), devices.release());
}
