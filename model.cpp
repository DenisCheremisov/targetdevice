#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <memory>
#include <vector>
#include <set>
#include <stdexcept>

#include "model.hpp"
#include "commands.hpp"
#include "devices.hpp"
#include "conditions.hpp"
#include "locker.hpp"

using namespace std;

const char
*ID = "ID",
    *COMMAND = "COMMAND",
    *NAME = "NAME",
    *COUPLE = "COUPLE",
    *START = "START",
    *STOP = "STOP",
    *COUPLING_INTERVAL = "COUPLING-INTERVAL",
    *RESTART = "RESTART",
    *CONDITION = "CONDITION",
    *TYPE = "TYPE",
    *OK = "OK";


string rtrim(string s) {
    size_t i = s.length();
    while(i > 0 && (s[i] == ' ' || s[i] == '\n')) {
        i--;
    }
    return string(s, i);
}


string ConfigInfoModel::execute(model_call_params_t &params)
    throw(InteruptionHandling) {
    string request_data = params.request_data;
    rtrim(request_data);

    if(request_data.length() == 0) {
        throw InteruptionHandling("No method requested, may be you meant GET?");
    } else if(params.request_data != "GET") {
        stringstream buf;
        buf << "Unknown method " << params.request_data << " requested";
        throw InteruptionHandling(buf.str());
    }

    return params.config->devices().view();
}


typedef pair<string, string> spair;


int pair_split(string &source, const char delim, spair &dest) {
    size_t pos = source.find(delim);
    if(pos == string::npos) {
        return -1;
    }

    dest = spair(source.substr(0, pos),
                 source.substr(pos + 1, source.length() - pos));
    return 0;
}


void BaseInstructionLine::deconstruct(string source, s_map &dest) {
    stringstream buf(source);
    string item;
    while(getline(buf, item, ':')) {
        spair data;
        if(pair_split(item, '=', data) < 0) {
            stringstream buf;
            buf << "Wrong fragment \"" << item << "\" in an instruction \"" <<
                source << '"';
            throw InteruptionHandling(buf.str());
        }
        dest[data.first] = data.second;
    }
}


void key_required(s_map &src, string key) {
    s_map::iterator it = src.find(key);
    if(it == src.end()) {
        stringstream buf;
        buf << "Key " << key << " required";
        throw InteruptionHandling(buf.str());
    }
}


ValueInstructionLine::ValueInstructionLine(s_map &ref) {
    key_required(ref, ID);
    key_required(ref, COMMAND);

    id = ref[ID];
    command = ref[COMMAND];
}


int need_int(string value, string key, const char *custom = NULL) {
    char *p;
    const char *start = value.c_str();
    int res = strtol(start, &p, 10);
    if(*p || p == start) {
        stringstream buf;
        if(custom != (const char*)NULL) {
            buf << custom;
        } else {
            buf << value << " is not a number in " << key << "=" << value;
        }
        throw InteruptionHandling(buf.str());
    }
    return res;
}


SingleInstructionLine::SingleInstructionLine(s_map &ref) {
    key_required(ref, ID);
    key_required(ref, COMMAND);
    key_required(ref, NAME);
    key_required(ref, START);

    s_map::iterator finder;

    finder = ref.find(STOP);
    if(finder != ref.end()) {
        stop = need_int(ref[STOP], STOP);
    } else {
        stop = -1;
    }

    finder = ref.find(RESTART);
    if(finder != ref.end()) {
        restart = need_int(ref[RESTART], STOP);
    } else {
        restart = -1;
    }

    id = ref[ID];
    command = ref[COMMAND];
    name = ref[NAME];
    start = need_int(ref[START], START);
}


CoupledInstructionLine::CoupledInstructionLine(s_map &ref):
    SingleInstructionLine(ref) {
    key_required(ref, COUPLE);
    key_required(ref, COUPLING_INTERVAL);

    couple = ref[COUPLE];
    coupling_interval = need_int(ref[COUPLING_INTERVAL], COUPLING_INTERVAL);
}


comparison_t parse_comparison(string source) throw(InteruptionHandling) {
    spair data;
    comparison_t comp;

    if(pair_split(source, '.', data) < 0) {
        stringstream buf;
        buf << "Wrong condition format: " << source;
        throw InteruptionHandling(buf.str());
    }
    comp.source = data.first;

    string tmp = data.second;
    if(pair_split(tmp, '.', data) < 0) {
        stringstream buf;
        buf << "Wrong condition format: " << source;
        throw InteruptionHandling(buf.str());
    }
    if(data.first == "temperature") {
        comp.source_endpoint = ENDPOINT_TEMPERATURE;
    } else {
        stringstream buf;
        buf << "Endpoint " << data.first << " is not supported " <<
            source;
        throw InteruptionHandling(buf.str());
    }

    tmp = data.second;
    if(pair_split(tmp, '_', data) < 0) {
        stringstream buf;
        buf << "Wrong comparison format, must be <OPERATOR>_<VALUE>, got " <<
            tmp << " instead";
        throw InteruptionHandling(buf.str());
    }
    comp.value = need_int(data.second, CONDITION,
                          (data.second + ": not a number").c_str());


    map<string, operation_t> chooser;
    chooser["LT"] = COMPARISON_LT;
    chooser["LE"] = COMPARISON_LE;
    chooser["EQ"] = COMPARISON_EQ;
    chooser["GE"] = COMPARISON_GE;
    chooser["GT"] = COMPARISON_GT;

    map<string, operation_t>::iterator lookup = chooser.find(data.first);
    if(lookup == chooser.end()) {
        stringstream buf;
        buf << "Wrong operation: " << data.first;
        throw InteruptionHandling(buf.str());
    } else {
        comp.operation = lookup->second;
    }

    return comp;
}


ConditionInstructionLine::ConditionInstructionLine(s_map &ref) {
    key_required(ref, ID);
    key_required(ref, NAME);
    key_required(ref, COMMAND);
    key_required(ref, COUPLE);
    key_required(ref, CONDITION);
    key_required(ref, START);

    s_map::iterator finder;

    finder = ref.find(STOP);
    if(finder != ref.end()) {
        stop = need_int(ref[STOP], STOP);
    } else {
        stop = -1;
    }

    id = ref[ID];
    name = ref[NAME];
    command = ref[COMMAND];
    couple = ref[COUPLE];
    start = need_int(ref[START], START);

    comparison = parse_comparison(ref[CONDITION]);
}


class StringSet: public set<string> {
public:
    StringSet& operator<<(string operation) {
        insert(operation);
        return *this;
    }

    bool has(string operation) {
        StringSet::iterator it = find(operation);
        return it != end();
    }
};


class SupportedCommands: public map<device_type_t, StringSet> {
public:
    bool is_supported(device_type_t device_type, string operation) {
        SupportedCommands::iterator it = find(device_type);
        if(it == end()) {
            return false;
        }
        return it->second.has(operation);
    }

    SupportedCommands() {
        (*this)[DEVICE_BOILER] << "on" << "off" << "temperature";
        (*this)[DEVICE_SWITCHER] << "on" << "off";
        (*this)[DEVICE_THERMOSWITCHER] << "temperature";
    };

};

SupportedCommands supported_commands;


Command *command_from_string(model_call_params_t &params, string cmd) {
    spair couple;
    if(pair_split(cmd, '.', couple) < 0) {
        throw InteruptionHandling(string("Wrong command: ") + cmd);
    }

    config_devices_t::const_iterator it =
        params.config->devices().find(couple.first);
    if(it == params.config->devices().end()) {
        stringstream buf;
        buf << "No such device: " << couple.first;
        throw InteruptionHandling(buf.str());
    }

    if(!supported_commands.is_supported(it->second->id(), couple.second)) {
        stringstream buf;
        buf << "Device \"" << couple.first << "\" does not support operation \"" <<
            couple.second << '"';
        throw InteruptionHandling(buf.str());
    }

    if(couple.second == "on") {
        try {
            return new SwitcherOn(params.devices->device(couple.first));
        } catch(CommandSetupError e) {
            stringstream buf;
            buf << "Device " << couple.first << " is not a switcher";
            throw InteruptionHandling(buf.str());
        }
    } else if(couple.second == "off") {
        try {
            return new SwitcherOff(params.devices->device(couple.first));
        } catch(CommandSetupError e) {
            stringstream buf;
            buf << "Device " << couple.first << " is not a switcher";
            throw InteruptionHandling(buf.str());
        }
    } else if(couple.second == "temperature") {
        try {
            return new TemperatureGet(params.devices->device(couple.first));
        } catch(CommandSetupError e) {
            stringstream buf;
            buf << "Device " << couple.first << " cannot take a temperature";
            throw InteruptionHandling(buf.str());
        }
    } else {
        throw InteruptionHandling("That should not happen");
    }
}


template <class T>
class safe_vector: public vector<T*> {
public:
    ~safe_vector() throw() {
        while(!this->empty()) {
            delete this->back();
            this->pop_back();
        }
    }
};


template <typename K, typename T>
class safe_map: public map<K, T*> {
public:
    ~safe_map() throw() {
        for(typename safe_map<K, T>::iterator it = this->begin();
            it != this->end(); it++) {
            if(it->second != NULL) {
                delete it->second;
            }
        }
    }
};


BaseSchedule *get_single(model_call_params_t &params,
                         SingleInstructionLine *item) {
    unique_ptr<Command> cmd(command_from_string(params, item->command));
    BaseSchedule *res =  new SingleCommandSchedule(
        cmd.get(), item->start, item->stop, item->restart);
    cmd.release();
    return res;
}


BaseSchedule *get_coupled(model_call_params_t &params,
                          CoupledInstructionLine *item) {
    unique_ptr<Command> cmd(command_from_string(params, item->command));
    unique_ptr<Command> couple(command_from_string(params, item->couple));
    BaseSchedule *res = new CoupledCommandSchedule(
        cmd.get(), item->start, item->stop,
        couple.get(), item->coupling_interval,
        item->restart);
    cmd.release();
    couple.release();
    return res;
}


BaseSchedule* get_conditioned(model_call_params_t &params,
                              ConditionInstructionLine *item) {
    unique_ptr<BaseCondition> cond;
    try {
        device_reference_t *devref = params.devices->device(
            item->comparison.source);
        DeviceTemperature *dvc = dynamic_cast<DeviceTemperature*>(
            devref->basepointer);
        if(dvc == (DeviceTemperature*)NULL) {
            stringstream buf;
            buf << item->comparison.source << " has no temperature port";
            throw InteruptionHandling(buf.str());
        }

        bool (*comparator)(double, double);
        switch(item->comparison.operation) {
        case COMPARISON_LT:
            comparator = compare_lt<double>;
            break;
        case COMPARISON_LE:
            comparator = compare_le<double>;
            break;
        case COMPARISON_EQ:
            comparator = compare_eq<double>;
            break;
        case COMPARISON_GE:
            comparator = compare_ge<double>;
            break;
        case COMPARISON_GT:
            comparator = compare_gt<double>;
            break;
        default:
            comparator = compare_lt<double>;
        }
        cond = unique_ptr<BaseCondition>(
            new TemperatureCondition(dvc, item->comparison.value, comparator));
    } catch(out_of_range e) {
        stringstream buf;
        buf << item->comparison.source << ": no such device";
        throw InteruptionHandling(buf.str());
    }

    unique_ptr<Command> cmd(command_from_string(params, item->command));
    unique_ptr<Command> couple(command_from_string(params, item->couple));

    BaseSchedule *res = new ConditionedSchedule(
        cmd.release(), couple.release(), cond.release(),
        item->start, item->stop);
    return res;
}


std::string resp_item(string id, bool status, string body) {
    stringstream buf;
    buf << "ID=" << id << ":SUCCESS=" << status;
    if(status) {
        buf << ":VALUE=" << body;
    } else {
        buf << ":ERROR=" << body;
    }
    return buf.str();
}


string InstructionListModel::execute(model_call_params_t &params)
    throw(InteruptionHandling) {
    stringstream buf(params.request_data);
    string line;

    safe_vector<ValueInstructionLine> values;
    safe_vector<SingleInstructionLine> singles;
    safe_vector<CoupledInstructionLine> couples;
    safe_vector<ConditionInstructionLine> conditionals;
    auto resources = params.res;

    stringstream results, error_results, drop_results;
    while(getline(buf, line, '\n')) {
        s_map ref;
        BaseInstructionLine::deconstruct(line, ref);
        try {
            key_required(ref, TYPE);
            key_required(ref, ID);
            string type = ref[TYPE];
            auto &&it = params.sched->find(ref[NAME]);
            if(it != params.sched->end() && type != "DROP") {
                stringstream buf;
                buf << "Task name=" << ref[NAME] <<
                    " has taken up already, may be drop it?";
                throw InteruptionHandling(buf.str());
            }
            if(type == "VALUE") {
                unique_ptr<ResourcesTaker> taker(resources->taker());
                key_required(ref, COMMAND);
                taker->take(ref[COMMAND]);
                values.push_back(new ValueInstructionLine(ref));
                taker->approve();
            } else if(type == "SINGLE") {
                unique_ptr<ResourcesTaker> taker(resources->taker());
                taker->take(ref[COMMAND]);
                (*params.busy)[ref[NAME]] = taker->captured();
                singles.push_back(new SingleInstructionLine(ref));
                taker->approve();
            } else if(type == "COUPLED") {
                unique_ptr<ResourcesTaker> taker(resources->taker());
                key_required(ref, COMMAND);
                key_required(ref, COUPLE);
                taker->take(ref[COMMAND]);
                taker->take(ref[COUPLE]);
                (*params.busy)[ref[NAME]] = taker->captured();
                couples.push_back(new CoupledInstructionLine(ref));
                taker->approve();
            } else if(type == "CONDITIONED") {
                unique_ptr<ResourcesTaker> taker(resources->taker());
                key_required(ref, COMMAND);
                key_required(ref, COUPLE);
                taker->take(ref[COMMAND]);
                taker->take(ref[COUPLE]);
                (*params.busy)[ref[NAME]] = taker->captured();
                conditionals.push_back(new ConditionInstructionLine(ref));
                taker->approve();
            } else if(type == "DROP") {
                if(it != params.sched->end()) {
                    UnifiedLocker<NamedSchedule> safe(params.sched);
                    safe->drop_schedule(ref[NAME]);
                    for(auto &it: params.busy->at(ref[NAME])) {
                        resources->release(it);
                    }
                }
                drop_results << resp_item(std::string("DROP.") + ref[ID],
                                              true, "dropped") << "\n";
            }
        } catch(InteruptionHandling e) {
            string id;
            try {
                id = ref.at("ID");
            } catch(out_of_range e) {
                id = "VOID";
            }
            error_results << resp_item(id, false, e) << endl;
        } catch(ResourceIsBusy e) {
            stringstream buf;
            buf << "Unable to take resource " << e.what() << "\n";
            error_results << resp_item(ref[ID], false, buf.str());
        }
    }

    // Now create schedules
    safe_map<string, BaseSchedule> res;

    for(safe_vector<SingleInstructionLine>::iterator it = singles.begin();
        it != singles.end(); it++) {
        SingleInstructionLine *item = *it;
        try {
            res[item->name] = get_single(params, item);
            results << resp_item(item->id, true, OK) << endl;
        } catch(ScheduleSetupError er) {
            error_results << resp_item(item->id, false, er.what()) << endl;
        }
    }

    for(safe_vector<CoupledInstructionLine>::iterator it = couples.begin();
        it != couples.end(); it++) {
        CoupledInstructionLine *item = *it;
        try {
            res[item->name] = get_coupled(params, item);
            results << resp_item(item->id, true, OK) << endl;
        } catch(ScheduleSetupError er) {
            error_results << resp_item(item->id, false, er.what()) << endl;
        }
    }

    for(safe_vector<ConditionInstructionLine>::iterator it = conditionals.begin();
        it != conditionals.end(); it++) {
        ConditionInstructionLine *item = *it;

        try {
            res[item->name] = get_conditioned(params, item);
            results << resp_item(item->id, true, OK) << endl;
        } catch(ScheduleSetupError er) {
            error_results << resp_item(item->id, false, er.what()) << endl;
        }
    }

    {
        UnifiedLocker<NamedSchedule> safe(params.sched);
        for(safe_map<string, BaseSchedule>::iterator it = res.begin();
            it != res.end(); it++) {
            safe->set_schedule(it->first, it->second);
            it->second = NULL;
        }
    }

    for(safe_vector<ValueInstructionLine>::iterator it = values.begin();
        it != values.end(); it++) {
        ValueInstructionLine *item = *it;
        try {
            unique_ptr<Command> cmd(command_from_string(params, item->command));
            unique_ptr<Result> value(cmd->execute());
            if(dynamic_cast<ErrorResult*>(value.get())) {
                results << resp_item(item->id, false, value->value());
            } else {
                results << resp_item(item->id, true, value->value());
            }
        } catch(InteruptionHandling er) {
            results << resp_item(item->id, false, er);
        }
        results << endl;
    }

    stringstream result_buf;
    result_buf << drop_results.str();
    result_buf << results.str();
    result_buf << error_results.str();
    return result_buf.str();
}
