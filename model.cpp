#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <memory>

#include "model.hpp"

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
    *CONDITION = "CONDITION";


string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(),
                    not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}


std::string ConfigInfoModel::execute(model_call_params_t &params)
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
    while(std::getline(buf, item, ':')) {
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


int need_int(string value, string key) {
    char *p;
    int res = strtol(value.c_str(), &p, 10);
    if(*p) {
        stringstream buf;
        buf << value << " is not a number in " << key << "=" << value;
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
