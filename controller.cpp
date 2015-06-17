#include <stdexcept>
#include <memory>

#include "controller.hpp"


using namespace std;

string Controller::greetings() {
    stringstream buf;
    buf << "READY" << "\n";
    buf << "IDENTITY=" << config->connection().identity << "\n";
    buf << "VERSION=" << Config::version << "\n";
    buf << "STARTUP=" << Config::startup << "\n";
    buf << "CONFIG=" << Config::conf_change << "\n";
    buf << "CONFDIGEST=" << Config::md5hexdigest << "\n";

    return buf.str();
}


BaseConnection* Controller::get_connection() {
    auto_ptr<Connection> conn(new Connection);
    conn->set_connection(config->connection().host,
                         config->connection().port);

    return conn.release();
}


class ModelMap: public map<string, BaseModel*> {
public:
    ~ModelMap() {
        for(ModelMap::iterator it = this->begin();
            it != this->end(); it++) {
            delete it->second;
        }
    }

    ModelMap() {
        (*this)["CONFIG"] = new ConfigInfoModel;
        (*this)["INSTRUCTIONS"] = new InstructionListModel;
    }
};


ModelMap models;


std::string response(bool success, string body) {
    stringstream buf;
    buf << "SUCCESS=" << success << "\n";
    buf << body;
    return buf.str();
}


void Controller::execute() {
    auto_ptr<BaseConnection> conn(get_connection());

    conn->send(greetings());

    string data = conn->receive();
    size_t pos = data.find('\n');
    string req_type;
    string req_data;
    string result;

    if(pos == string::npos) {
        req_type = data;
        req_data = "";
    } else {
        req_type = data.substr(0, pos);
        req_data = data.substr(pos + 1, data.length() - pos - 1);
    }

    try {
        BaseModel *model = models.at(req_type);
        model_call_params_t params;
        params.config = config;
        params.devices = devices;
        params.sched = sched;
        params.request_data = req_data;
        params.res = resources;
        params.busy = this->busy_resources;
        try {
            result = response(true, model->execute(params));
        } catch(InteruptionHandling e) {
            result = response(false, e);
        } catch(exception &e) {
            result = response(false, e.what());
        }
    } catch(out_of_range e) {
        result = response(false, req_type + ": operation not supported");
    }

    conn->send(result);
}
