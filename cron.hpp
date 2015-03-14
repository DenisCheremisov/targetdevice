#ifndef _CRON_HPP_DEFINED_
#define _CRON_HPP_DEFINED_

#include <vector>
#include <string>
#include <climits>

class BaseMatcher {
public:
    virtual ~BaseMatcher() throw() {};

    virtual bool match(int value) throw() = 0;
};


class MatcherWrongInitValue: public std::exception, std::string {
public:
    ~MatcherWrongInitValue() throw() {};
    MatcherWrongInitValue(const std::string &msg): std::string(msg) {};

    const char *what() throw() {
        return this->c_str();
    }
};


class EnumMatcherWrongInitValue: public MatcherWrongInitValue {
public:
    ~EnumMatcherWrongInitValue() throw() {};
    EnumMatcherWrongInitValue(const std::string &msg): MatcherWrongInitValue(msg) {};
};


class RangeMatcherWrongInitValue: public MatcherWrongInitValue {
public:
    ~RangeMatcherWrongInitValue() throw() {};
    RangeMatcherWrongInitValue(const std::string &msg): MatcherWrongInitValue(msg) {};
};


class EnumMatcher: public BaseMatcher, public std::vector<int> {
private:
    int limit;

public:
    ~EnumMatcher() throw() {};
    EnumMatcher(int lim = INT_MAX): limit(lim) {};

    EnumMatcher& operator<<(int value) throw(EnumMatcherWrongInitValue);

    bool match(int value) throw();
};


class RangeMatcher: public BaseMatcher {
private:
    int begin, end, step;
    int limit;

public:
    ~RangeMatcher() throw() {};
    RangeMatcher(int lim = INT_MAX) {
        begin = 0;
        end = -1;
        step = 1;
        limit = lim;
    }

    void set_range(int b, int e = -1, int s = 1) throw(RangeMatcherWrongInitValue);

    bool match(int value) throw();
};


#endif
