#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include "model.hpp"


using namespace std;


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
