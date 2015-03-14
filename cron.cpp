#include <sstream>

#include "cron.hpp"


using namespace std;


EnumMatcher& EnumMatcher::operator<<(int value) throw(EnumMatcherWrongInitValue) {
    if(value >= limit) {
        stringstream buf;
        buf << "Got " << value << " which is above the limit set: " << limit;
        throw EnumMatcherWrongInitValue(buf.str());
    }
    if(this->size() > 0) {
        int last_value = (*this)[this->size() - 1];
        if(value <= last_value) {
            stringstream buf;
            buf << "Given value " << value << " is not larger than the previous: " <<
                last_value;
            throw EnumMatcherWrongInitValue(buf.str());
        }
    }
    this->push_back(value);
    return *this;
}


bool EnumMatcher::match(int value) throw() {
    if(value >= limit) {
        return false;
    }
    for(int i = 0; i < this->size(); i++) {
        if((*this)[i] == value) {
            return true;
        }
    }
    return false;
};


void RangeMatcher::set_range(int b, int e, int s) throw(RangeMatcherWrongInitValue) {
    if(b >= limit) {
        stringstream buf;
        buf << "Got lower bound " << b << ", it must be lower than " << limit;
        throw RangeMatcherWrongInitValue(buf.str());
    }
    if(e >= limit) {
        stringstream buf;
        buf << "Got upper bound " << e << ", it must be lower than" << limit;
        throw RangeMatcherWrongInitValue(buf.str());
    }
    if(b < 0) {
        stringstream buf;
        buf << "Lower bound of a range must not be negative, got " << b;
    }
    if(s <= 0) {
        stringstream buf;
        buf << "Step must be larger than 0, got " << s;
        throw RangeMatcherWrongInitValue(buf.str());
    }
    if(e >= 0 && b > e) {
        stringstream buf;
        buf << "Lower bound must not be larger than upper, got " << b << ">" << e;
        throw RangeMatcherWrongInitValue(buf.str());
    }
    begin = b;
    end = e;
    step = s;
}


bool RangeMatcher::match(int value) throw() {
    if(value >= limit || value < begin || (value > end && end >= 0)) {
        return false;
    }
    return (value - begin) % step == 0;
}
