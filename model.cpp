#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <memory>
#include <vector>
#include <set>

#include "model.hpp"
#include "commands.hpp"
#include "devices.hpp"

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
    *TYPE = "TYPE";


string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(),
                    not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
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

    stringstream buf;
    for(Devices::iterator it = params.devices->begin();
        it != params.devices->end(); it++) {
        buf << it->first << ":";
        switch(it->second->type) {
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
        buf << "No key " << key << " found";
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
    chooser["LTE"] = COMPARISON_LTE;
    chooser["EQ"] = COMPARISON_EQ;
    chooser["GTE"] = COMPARISON_GTE;
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
        buf << "Device " << couple.first << " does not support " <<
            couple.second << " operation";
        throw InteruptionHandling(buf.str());
    }

    if(couple.second == "on") {
        return new SwitcherOn(params.devices->device(couple.first)->switcher);
    } else if(couple.second == "off") {
        return new SwitcherOff(params.devices->device(couple.first)->switcher);
    } else if(couple.second == "temperature") {
        return new
            TemperatureGet(params.devices->device(couple.first)->temperature);
    } else {
        throw InteruptionHandling("That should not happen");
    }
}


string InstructionListModel::execute(model_call_params_t &params)
    throw(InteruptionHandling) {
    stringstream buf(params.request_data);
    string line;
    while(getline(buf, line, '\n')) {
        s_map ref;
        BaseInstructionLine::deconstruct(line, ref);

        key_required(ref, TYPE);
        string type = ref[TYPE];

        vector<ValueInstructionLine *> values;
        vector<SingleInstructionLine *> singles;
        vector<CoupledInstructionLine *> couples;
        vector<ConditionInstructionLine *> conditionals;
        if(type == "VALUE") {
            values.push_back(new ValueInstructionLine(ref));
        } else if(type == "SINGLE") {
            singles.push_back(new SingleInstructionLine(ref));
        } else if(type == "COUPLE") {
            couples.push_back(new CoupledInstructionLine(ref));
        } else if(type == "CONDITION") {
            conditionals.push_back(new ConditionInstructionLine(ref));
        }
    }

    return "";
}
