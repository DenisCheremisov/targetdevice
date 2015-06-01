#include "drivers.hpp"

#include <regex>
#include <vector>
#include <iomanip>


using namespace std;


const string STRUCTURE_ERROR = "#ERR";


class MismatchError: public exception {
public:
    ~MismatchError() throw() {};
};


class NumMatcher {
private:
    regex *r;

public:
    NumMatcher() {
        r = new regex("[[:digit:]]+");
    }
    ~NumMatcher() {
        delete r;
    }
    void match(const string &val) throw(MismatchError) {
        if(!regex_match(val, *r)) {
            throw MismatchError();
        }
    }
};
NumMatcher num_matcher;


template <int LOWER, int UPPER>
class RangeMatcher: public NumMatcher {
public:
    int match(const string &val) throw(MismatchError) {
        num_matcher.match(val);
        int value = stoi(val);
        if(!(LOWER <= value && value <= UPPER)) {
            throw MismatchError();
        }
        return value;
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


template <class T>
void expect_length(const T &array, size_t length) throw(MismatchError) {
    if(array.size() != length) {
        throw MismatchError();
    }
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
    vector<string> &data = *(tokens.get());
    if(tokens->size() < 1) {
        return STRUCTURE_ERROR;
    }

    string res;
    try {
        if(data[0] != "$KE") {
            throw MismatchError();
        }
        if(data.size() == 1) {
            res = "#OK";
        } else if(data[1] == "WR") {
            expect_length(data, 4);
            auto val1 = RangeMatcher<1, 18>().match(data[2]);
            auto val2 = RangeMatcher<0, 1>().match(data[3]);
            res = write_line(val1, val2);
        } else if(data[1] == "RD"){
            expect_length(data, 3);
            if(data[2] == "ALL") {
                res = read_all_lines();
            } else {
                auto val = RangeMatcher<1, 18>().match(data[2]);
                res = read_line(val);
            }
        } else if(data[1] == "IO") {
            if(data[2] == "GET") {
                if(data[3] != "MEM" && data[3] != "CUR") {
                    throw MismatchError();
                }
                switch(data.size()) {
                case 4:
                    res = ioget_all();
                    break;
                case 5: {
                    auto val = RangeMatcher<1, 18>().match(data[4]);
                    res = ioget(val);
                    break;
                }
                default:
                     MismatchError();
                }
            } else if(data[2] == "SET") {
                if(data.size() == 6) {
                    if(data[5] != "S") {
                        throw MismatchError();
                    }
                } else {
                    expect_length(data, 5);
                }
                auto val1 = RangeMatcher<1, 18>().match(data[3]);
                auto val2 = RangeMatcher<0, 1>().match(data[4]);
                res = ioset(val1, val2);
            } else {
                throw MismatchError();
            }
        } else if(data[1] == "REL") {
            expect_length(data, 4);
            auto val1 = RangeMatcher<1, 4>().match(data[2]);
            auto val2 = RangeMatcher<0, 1>().match(data[3]);
            res = write_relay(val1, val2);
        } else if(data[1] == "ADC") {
            expect_length(data, 3);
            auto val = RangeMatcher<1, 4>().match(data[2]);
            res = get_adc(val);
        } else if(data[1] == "AFR") {
            expect_length(data, 3);
            auto val = RangeMatcher<0, 400>().match(data[2]);
            res = set_freq(val);
        }
        if(res == "") {
            throw MismatchError();
        }
    } catch(MismatchError) {
        return STRUCTURE_ERROR;
    } catch(...) {
        return STRUCTURE_ERROR;
    }

    return res;
}
