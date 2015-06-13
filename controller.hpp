#ifndef _CONTROLER_HPP_INCLUDED_
#define _CONTROLER_HPP_INCLUDED_


#include "confparser.hpp"
#include "confbind.hpp"
#include "network.hpp"
#include "model.hpp"
#include "resourcemanager.hpp"


class Controller {
private:
    Config *config;
    Devices *devices;
    NamedSchedule *sched;
    Resources *resources;
    std::map<std::string, std::list<std::string> > *busy_resources;

public:
    Controller(Config *_conf,
               Devices *_devices,
               NamedSchedule *_sched,
               Resources *_res):
        config(_conf), devices(_devices), sched(_sched), resources(_res) {
        busy_resources = new std::map<std::string,
                                      std::list<std::string> >;
    };
    virtual ~Controller() throw() {
        delete busy_resources;
    };

    virtual BaseConnection* get_connection();

    std::string greetings();

    void execute();
};

#endif
