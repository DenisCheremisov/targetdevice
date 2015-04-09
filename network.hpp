#ifndef _NETWORK_HPP_INCLUDED_
#define _NETWORK_HPP_INCLUDED_

#include <string>

#include <openssl/ssl.h>


class BaseConnection {
public:
    virtual ~BaseConnection() throw() {};
    virtual bool connected() = 0;
    virtual void send(std::string) = 0;
    virtual std::string receive() = 0;
};


std::string get_ssl_error();
std::string get_system_error();


class ConnectionError: public std::exception, public std::string {
public:
    ~ConnectionError() throw() {};
    ConnectionError(std::string message): std::string(message) {};

    const char *what() const throw() {
        return this->c_str();
    }
};


class Connection: public BaseConnection {
private:
    int socket;
    SSL *ssl_handle;
    SSL_CTX *ssl_context;

public:
    Connection(): socket(0), ssl_handle(NULL), ssl_context(NULL) {};
    virtual ~Connection() throw();

    void set_connection(std::string host, int port);
    virtual bool connected();
    void send(std::string);
    std::string receive();
};


#endif
