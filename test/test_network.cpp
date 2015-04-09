#define BOOST_TEST_IGNORE_SIGKILL
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE NetworkTestModule

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#include <memory>
#include <boost/test/unit_test.hpp>

#include "../network.hpp"

#define FAIL    -1

using namespace std;



BOOST_AUTO_TEST_CASE(test_network_connector) {
    FILE *fp;
    std::stringstream buf;
    buf << "python scripts/is-connected-server.py ";
    fp = popen(buf.str().c_str(), "r");
    if(fp == NULL) {
        throw "Cannot start the virtual serial device";
    }

    SSL_load_error_strings();
    SSL_library_init();

    Connection conn;

    conn.set_connection("localhost", 10023);
    conn.send("hello");
    BOOST_CHECK_EQUAL(conn.receive(), "0:error-intro");

    Connection conn2;
    BOOST_REQUIRE_THROW(conn2.set_connection("localhost", 50001),
                        ConnectionError);
}
