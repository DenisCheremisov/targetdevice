#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CronEmuModule

#include <boost/test/unit_test.hpp>

#include "../cron.hpp"


BOOST_AUTO_TEST_CASE(test_enum_matcher) {
    EnumMatcher matcher;

    matcher << 1 << 5 << 25 << 500 << 1000;

    BOOST_REQUIRE_THROW(matcher << 999,
                        EnumMatcherWrongInitValue);
    BOOST_CHECK(matcher.match(1));
    BOOST_CHECK(matcher.match(5));
    BOOST_CHECK(matcher.match(25));
    BOOST_CHECK(matcher.match(500));
    BOOST_CHECK(matcher.match(1000));
    BOOST_CHECK_EQUAL(matcher.size(), 5);

    BOOST_CHECK_EQUAL(matcher.match(499), false);
    BOOST_CHECK_EQUAL(matcher.match(501), false);


    // Testing wrong init

    EnumMatcher matcher2(30);

    matcher2 << 1 << 2 << 5 << 10;
    BOOST_REQUIRE_THROW(matcher2 << 30,
                        EnumMatcherWrongInitValue);
    BOOST_REQUIRE_THROW(matcher2 << 35,
                        EnumMatcherWrongInitValue);
}


BOOST_AUTO_TEST_CASE(test_range_matcher) {
    RangeMatcher matcher1;

    for(int i = 0; i < 10000; i++) {
        BOOST_CHECK_EQUAL(matcher1.match(i), true);
    }

    RangeMatcher matcher2;
    matcher2.set_range(0, -1, 2);
    for(int i = 0; i < 10000; i += 2) {
        BOOST_CHECK_EQUAL(matcher2.match(i), true);
        BOOST_CHECK_EQUAL(matcher2.match(i + 1), false);
    }

    RangeMatcher matcher3(24);
    matcher3.set_range(5, 11, 3);
    BOOST_CHECK_EQUAL(matcher3.match(5), true);
    BOOST_CHECK_EQUAL(matcher3.match(6), false);
    BOOST_CHECK_EQUAL(matcher3.match(7), false);
    BOOST_CHECK_EQUAL(matcher3.match(8), true);
    BOOST_CHECK_EQUAL(matcher3.match(9), false);
    BOOST_CHECK_EQUAL(matcher3.match(10), false);
    BOOST_CHECK_EQUAL(matcher3.match(11), true);
    for(int i = 12; i < 10000; i++) {
        BOOST_CHECK_EQUAL(matcher3.match(i), false);
    }

    BOOST_REQUIRE_THROW(matcher3.set_range(25, 26, 1),
                        RangeMatcherWrongInitValue);
    BOOST_REQUIRE_THROW(matcher3.set_range(2, 26, 1),
                        RangeMatcherWrongInitValue);
    BOOST_REQUIRE_THROW(matcher3.set_range(2, 20, 0),
                        RangeMatcherWrongInitValue);
    BOOST_REQUIRE_THROW(matcher3.set_range(2, 20, -1),
                        RangeMatcherWrongInitValue);
    BOOST_REQUIRE_THROW(matcher3.set_range(2, 1, -1),
                        RangeMatcherWrongInitValue);
    BOOST_REQUIRE_THROW(matcher3.set_range(25, -1, 2),
                        RangeMatcherWrongInitValue);
}
