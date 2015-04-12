#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <bsd/libutil.h>
#include <sys/syslog.h>
#include <pthread.h>
#include <err.h>

#include <string>
#include <exception>
#include <memory>

#include "confparser.hpp"
#include "confbind.hpp"
#include "runtime.hpp"
#include "background.hpp"
#include "controller.hpp"


using namespace std;

const char *CONFIG_FILE_NAME = "conf/targetdevice.yaml";


class GlobalDataInitializer {
public:
    Config* conf;
    Devices* devices;
    Drivers *drivers;
    NamedSchedule *sched;

    GlobalDataInitializer(const char *config_file_name) {
        FILE *fp = open_conf_fp(config_file_name);
        if(fp == NULL) {
            perror(config_file_name);
            errx(
                EXIT_FAILURE,
                "Cannot open configuration file: %s",
                config_file_name);
        }
        auto_ptr<MapElement> res(dynamic_cast<MapElement*>(raw_conf_parse(fp)));
        conf = config_parse(res.get());

        drivers = new Drivers(conf->drivers());
        devices = new Devices(*drivers, conf->devices());
        sched = new NamedSchedule;

        // Switching off devices
        for(Devices::iterator it = devices->begin();
            it != devices->end(); it++) {
            try {
                SwitcherOff command(it->second);
                command.execute();
            } catch(CommandSetupError e) {
                ; // Do nothing, we are just setting them all off wherever possible
            }
        }
    };

    ~GlobalDataInitializer() throw() {
        delete conf;
        delete devices;
        delete drivers;
        delete sched;
    }
};


int main(int argc, char **argv) {
    char *config_file_name, *pidfile, *data;
    pid_t otherpid, childpid;
    struct pidfh *pfh;

    bool daemonize = false;

    for(int i = 1; i < argc; i++) {
        if(string(argv[i]) == "--daemonize") {
            daemonize = true;
        }
    }

    config_file_name = NULL;
    for(int i = 1; i < argc; i++) {
        if(string(argv[i]) == "--conf") {
            if(i == (argc - 1)) {
                errx(EXIT_FAILURE, "No config file name after --conf");
            }
            config_file_name = argv[i + 1];
        }
    }
    auto_ptr<GlobalDataInitializer> init;
    try {
        auto_ptr<GlobalDataInitializer> _init(
            new GlobalDataInitializer(
                config_file_name?config_file_name:CONFIG_FILE_NAME));
        init = _init;
    } catch(exception &err) {
        errx(EXIT_FAILURE, err.what());
    }

    if(daemonize) {
        const char *pidfile = init->conf->daemon().pidfile.c_str();
        pfh = pidfile_open(pidfile, 0600, &otherpid);
        if(pfh == NULL) {
            if(errno == EEXIST) {
                errx(EXIT_FAILURE, "Daemon already running, pidf: %jd.",
                     (intmax_t)otherpid);
            }
            warn("Cannot open or create pidfile");
        }

        if(daemon(0, 0) == -1) {
            warn("Cannot daemonize");
            pidfile_remove(pfh);
            exit(EXIT_FAILURE);
        }

        pidfile_write(pfh);
        childpid = fork();

        switch(childpid) {
        case -1:
            syslog(LOG_ERR, "Cannot fork(): %s.", strerror(errno));
            return -1;
            break;
        case 0:
            pidfile_write(pfh);
            break;
        default:
            syslog(LOG_INFO, "Child %jd started.", (intmax_t)childpid);
            return 0;
            break;
        }
    }

    // Do work
    pthread_t thread;
    pthread_create(&thread, NULL, background_worker, &(init->sched));

    Controller controller(init->conf, init->devices, init->sched);
    while(true) {
        controller.execute();
        sleep(120);
    }

    return 0;
}
