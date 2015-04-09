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
        (*this)["INSTRUCTION"] = new InstructionListModel;
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

    istringstream data(conn->receive());
    string req_type;
    if(!getline(data, req_type, '\n')) {
        conn->send(response(false, "Wrong request"));
        return;
    }

    string result;
    try {
        BaseModel *model = models.at(req_type);
        model_call_params_t params;
        params.config = config;
        params.devices = devices;
        params.sched = sched;
        try {
            result = response(true, model->execute(params));
        } catch(InteruptionHandling e) {
            result = response(false, e);
        } catch(exception e) {
            result = response(false, e.what());
        }
    } catch(out_of_range e) {
        result = response(false, req_type + ": operation not supported");
    }

    conn->send(result);
}
