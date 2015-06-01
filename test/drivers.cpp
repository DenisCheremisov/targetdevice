#include "drivers.hpp"

#include <regex>
#include <vector>
#include <iomanip>


using namespace std;


const string STRUCTURE_ERROR = "#ERR";


class BaseMatcher {
public:
    ~BaseMatcher() throw() {};
    virtual bool match(const string &value) = 0;
};


class ConstMatcher: public BaseMatcher {
private:
    string value;

public:
    ConstMatcher(const string &val) {
        value = val;
    }
    ~ConstMatcher() throw() {};

    bool match(const string &val) {
        return val == value;
    }
};


class NumMatcher: public BaseMatcher {
private:
    regex *r;

public:
    NumMatcher() {
        r = new regex("[[:digit:]]+");
    }
    ~NumMatcher() throw() {
        delete r;
    };

    bool match(const string &val) {
        return regex_match(val, *r);
    }
};


vector<string> *tokenize(const string &source) {
    auto res = new vector<string>;
    string item;
    stringstream input(source);
    while(getline(input, item, ',')) {
        res->push_back(item);
    }
    return res;
}


TestSerialCommunicator::TestSerialCommunicator() {
    for(int i = 0; i < LINE_LIMIT; i++) {
        iodirs[i] = 0;
        lines[i] = 0;
    }
    lines[1] = 1;
    for(int i = 0; i < RELAY_LIMIT; i++) {
        relays[i] = 0;
    }
    for(int i = 0; i < ADC_LIMIT; i++) {
        adcs[i] = 0;
    }
    freq = 0;
    user_data = "TEST_DATA";
    usb_data = "TEST_USB_DATA";
}


string TestSerialCommunicator::is_connected() {
    return "#OK";
}


string TestSerialCommunicator::write_line(int lineno, int value) {
    if(iodirs[lineno - 1] == DIR_OUT)  {
        return "#WR,WRONGLINE";
    }
    lines[lineno - 1] = value;
    return "#WR,OK";
}


string TestSerialCommunicator::read_line(int lineno) {
    if(iodirs[lineno - 1] == DIR_IN) {
        return "#RD,WRONGLINE";
    }
    stringstream buf;
    buf << "#RD," << lineno << "," << lines[lineno - 1];
    return buf.str();
}


string TestSerialCommunicator::read_all_lines() {
    stringstream buf;
    buf << "#RD,";
    for(int i = 0; i < LINE_LIMIT; i++) {
        if(iodirs[i] == DIR_OUT) {
            buf << lines[i];
        } else {
            buf << "x";
        }
    }
    return buf.str();
}


string TestSerialCommunicator::write_relay(int relno, int value) {
    relays[relno - 1] = value;
    return "#REL,OK";
}


string TestSerialCommunicator::read_relay(int relno) {
    stringstream buf;
    buf << "#RID," << relno << "," << relays[relno - 1];
    return buf.str();
}


string TestSerialCommunicator::set_freq(int freqval) {
    freq = freqval;
    return "#AFR,OK";
}


string TestSerialCommunicator::get_adc(int channel) {
    int val = adcs[channel - 1];
    adcs[channel - 1] = (adcs[channel - 1] + 1) % 1024;
    stringstream buf;
    buf << "#ADC," << channel << ",";
    buf << setfill('0') << setw(4) << val;
    return buf.str();
}


string TestSerialCommunicator::ioset(int lineno, int direction) {
    iodirs[lineno - 1] = direction;
    return "#IO,SET,OK";
}


string TestSerialCommunicator::ioget(int lineno) {
    int value = iodirs[lineno - 1];
    stringstream buf;
    buf << "#IO," << value;
    return buf.str();
}


string TestSerialCommunicator::ioget_all() {
    stringstream buf;
    buf << "#IO,";
    for(int item: iodirs) {
        buf << item;
    }
    return buf.str();
}


string TestSerialCommunicator::set_ud(string data) {
    user_data = data;
    return "#UD,SET,OK";
}


string TestSerialCommunicator::get_ud() {
    stringstream buf;
    buf << "#UD," << user_data;
    return buf.str();
}


string TestSerialCommunicator::set_usb(string data) {
    usb_data = data;
    return "#USB,SET,OK";
}


string TestSerialCommunicator::get_usb() {
    stringstream buf;
    buf << "#USB," << usb_data;
    return buf.str();
}


string TestSerialCommunicator::get_ser() {
    stringstream buf;
    buf << "#SER," << SERIAL_DATA;
    return buf.str();
}


string TestSerialCommunicator::reset() {
    return "#RST,OK";
}


string TestSerialCommunicator::talk(string req) {
    if(req.size() < 2) {
        return STRUCTURE_ERROR;
    }
    if(req.substr(req.size() - 2, 2) != "\r\n") {
        return STRUCTURE_ERROR;
    }

    auto tokens = unique_ptr<vector<string>>(
        tokenize(req.substr(0, req.size() - 2)));
    if(tokens->size() < 1) {
        return STRUCTURE_ERROR;
    }
    if(!ConstMatcher("$KE").match((*(tokens.get()))[0])) {
        return STRUCTURE_ERROR;
    }
    return STRUCTURE_ERROR;
}
