#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <iomanip>

#include <unistd.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <openssl/md5.h>

#include <err.h>
#include <string>
#include <sstream>
#include <fstream>
#include <exception>
#include <memory>

#include <pthread.h>
#include <openssl/ssl.h>

#include "confparser.hpp"
#include "confbind.hpp"
#include "runtime.hpp"
#include "background.hpp"
#include "controller.hpp"
#include "resourcemanager.hpp"


using namespace std;

const char *CONFIG_FILE_NAME = "conf/targetdevice.yaml";


class GlobalDataInitializer {
public:
    Config* conf;
    Devices* devices;
    Drivers *drivers;
    NamedSchedule *sched;
    Resources *resources;

    GlobalDataInitializer(GlobalDataInitializer &&tmp) {
        conf = tmp.conf;
        devices = tmp.devices;
        drivers = tmp.drivers;
        sched = tmp.sched;
        resources = tmp.resources;
    }

    GlobalDataInitializer(const char *config_file_name) {
        FILE *fp = fopen(config_file_name, "r");
        if(fp == NULL) {
            perror(config_file_name);
            errx(
                EXIT_FAILURE,
                "Cannot open configuration file: %s",
                config_file_name);
        }
        unique_ptr<YamlParser> parser(YamlParser::get(fp));
        ConfigStruct rawconf;
        UserData userdata;
        yaml_parse(parser.get(), &rawconf);
        try {
            rawconf.check(&userdata);
        } catch(ParserError e) {
            cerr << e.what() << endl;
            exit(-1);
        }
        // Setting up resources
        resources = new Resources;
        for(DevicesStruct::iterator it = rawconf.devices.begin();
            it != rawconf.devices.end(); it++) {
            SerialDeviceStruct *device = dynamic_cast<SerialDeviceStruct*>(
                it->second);
            if(device == NULL) {
                continue;
            }
            if(device->type.value == "switcher") {
                resources->add_resource(it->first + ".on", it->first);
                resources->add_resource(it->first + ".off", it->first);
            } else if(device->type.value == "boiler") {
                resources->add_resource(it->first + ".on", it->first);
                resources->add_resource(it->first + ".off", it->first);
            }
        }
        conf = Config::get_from_struct(&rawconf);

        // Setting up config file last change timestamp
        struct stat buffer;
        stat(config_file_name, &buffer);
        Config::conf_change = (time_t)buffer.st_mtim.tv_sec;

        drivers = new Drivers(conf->drivers());
        devices = new Devices(*drivers, conf->devices());
        sched = new NamedSchedule;

        auto device_view = conf->devices().view();
        MD5_CTX md5handler;
        unsigned char md5digest[MD5_DIGEST_LENGTH];
        MD5_Init(&md5handler);
        MD5_Update(&md5handler, device_view.c_str(), device_view.length());
        MD5_Final(md5digest, &md5handler);
        stringstream md5hd;
        md5hd << setfill('0');
        for(auto i: md5digest) {
            md5hd << hex << setw(2) << (int)i;
        }
        Config::md5hexdigest = md5hd.str();

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

        SSL_library_init();
    };

    ~GlobalDataInitializer() throw() {
        delete conf;
        delete devices;
        delete drivers;
        delete sched;
        delete resources;
    }
};


class PidfileOpenError: public std::string {
public:
    PidfileOpenError(std::string msg): std::string(msg) {}
    PidfileOpenError(int code): std::string(strerror(code)) {}
};


class PidfileLockError: public std::string {
public:
    PidfileLockError(std::string msg): std::string(msg) {}
    PidfileLockError(int code): std::string(strerror(code)) {}
};


class PidfileOperator {
private:
    int fd;
    PidfileOperator(int _fd): fd(_fd) {}

public:
    static PidfileOperator *open(const char *filename) {
        int fd = ::open(filename,
                  O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK,
                  S_IRUSR | S_IWUSR);
        if(fd < 0) {
            throw PidfileOpenError(errno);
        }

        int status = flock(fd, LOCK_EX | LOCK_NB);
        if(status) {
            close(fd);
            std::ifstream file(filename);
            std::string buf;
            file >> buf;
            throw PidfileLockError(buf);
        }
        status = pwrite(fd, "-1", 2, 0);
        if(status < 0) {
            throw PidfileLockError(status);
        }

        return new PidfileOperator(fd);
    }

    ~PidfileOperator() throw() {
        if(fd > 0) {
            close(fd);
        }
    }

    void write(int pid) {
        std::stringstream buf;
        buf << pid;
        std::string data = buf.str();
        int status = pwrite(fd, data.c_str(), data.size(), 0);
        if(status < 0) {
            ;
        }
    }
};



class TableFormatter: public list<pair<string, string> > {
private:
    size_t max_head;

public:
    TableFormatter(): max_head(0) {};
    void add_raw(string head, string content) {
        if(max_head < head.size()) {
            max_head = head.size();
        }
        this->push_back(pair<string, string>(head, content));
    }

    string out_raw(const pair<string, string> &src) const {
        return src.first + string(max_head - src.first.size() + 4, ' ') +
            src.second;
    }
};


ostream& operator<<(ostream &dst, const TableFormatter &src) {
    for(TableFormatter::const_iterator it = src.begin();
        it != src.end(); it++) {
        dst << src.out_raw(*it) << endl;
    }
    return dst;
}


int main(int argc, char **argv) {
    pid_t childpid;

    bool daemonize = false;
    string config_file_name = CONFIG_FILE_NAME;
    int opt;

    while((opt = getopt(argc, argv, "hdc:")) != -1) {
        switch(opt) {
        case 'h': {
            cout << "Usage:" << endl;
            TableFormatter formatter;
            formatter.add_raw("-h", "produce help message");
            formatter.add_raw("-d", "start program as a daemon");
            formatter.add_raw(
                string("-c <arg> (=") +
                string(CONFIG_FILE_NAME) +
                ")", "configuration file path");
            cout << formatter << endl;
            return 0;
        }
        case 'd':
            daemonize = true;
            break;
        case 'c':
            config_file_name = optarg;
            break;
        case '?':
            if(optopt == 'c') {
                ;
            } else if(isprint(optopt)) {
                ;
            } else {
                cerr << "Unknown option character" << optopt << endl;
            }
            return -1;
        default:
            abort();
        }
    }

    GlobalDataInitializer init = [&]() -> GlobalDataInitializer {
        try {
            return GlobalDataInitializer(config_file_name.c_str());
        } catch(exception &err) {
            cerr << err.what() << endl;
            exit(EXIT_FAILURE);
        }
    }();

    unique_ptr<PidfileOperator> pidoperator;
    if(daemonize) {
        const char *pidfilename = init.conf->daemon().pidfile.c_str();
        PidfileOperator *pidop;

        try {
            pidop = PidfileOperator::open(pidfilename);
        } catch(PidfileOpenError e) {
            syslog(LOG_ERR, "Error: %s", e.c_str());
            return -1;
        } catch(PidfileLockError e) {
            syslog(
                LOG_ERR,
                "Cannot start daemon since another instance is running: %s",
                e.c_str());
            return -1;
        }

        if(daemon(0, 0) == -1) {
            warn("Cannot daemonize");
            delete pidop;
            exit(EXIT_FAILURE);
        }

        pidop->write(getpid());
        childpid = fork();

        switch(childpid) {
        case -1:
            syslog(LOG_ERR, "Cannot fork(): %s.", strerror(errno));
            return -1;
            break;
        case 0:
            pidoperator = unique_ptr<PidfileOperator>(pidop);
            pidop->write(getpid());
            break;
        default:
            syslog(LOG_INFO, "Child %d started.", (int)childpid);
            return 0;
            break;
        }
    }

    // Do work
    pthread_t thread;
    pthread_create(&thread, NULL, background_worker, init.sched);

    Controller controller(init.conf, init.devices, init.sched,
                          init.resources);
    while(true) {
        try {
            controller.execute();
        } catch(ConnectionError &e) {
            cout << "Cannot connect: " << e.what() << endl;
        }
        sleep(120);
    }

    return 0;
}
