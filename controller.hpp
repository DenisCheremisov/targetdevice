#ifndef _CONTROLER_HPP_INCLUDED_
#define _CONTROLER_HPP_INCLUDED_


#include "confparser.hpp"
#include "confbind.hpp"
#include "network.hpp"
#include "model.hpp"


class Controller {
private:
    Config *config;
    Devices *devices;
    NamedSchedule *sched;

public:
    Controller(Config *_conf,
               Devices *_devices,
               NamedSchedule *_sched,
               time_t _startup, time_t _conf_change):
        config(_conf), devices(_devices), sched(_sched) {};
    virtual ~Controller() throw() {};

    BaseConnection* get_connection();

    std::string greetings();

    void execute();
};

#endif
