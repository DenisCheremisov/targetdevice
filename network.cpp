#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sstream>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#include "network.hpp"


std::string get_ssl_error() {
    unsigned long error_code = ERR_get_error();
    return ERR_error_string(error_code, NULL);
}


std::string get_system_error() {
    return strerror(errno);
}


Connection::~Connection() throw() {
    if(socket) {
        close(socket);
    }
    if(ssl_handle) {
        SSL_shutdown(ssl_handle);
        SSL_free(ssl_handle);
    }
    if(ssl_context) {
        SSL_CTX_free(ssl_context);
    }
}


bool Connection::connected() {
    if(socket) {
        return true;
    } else {
        return false;
    }
}


int tcp_connect(const char *host_addr, int port) {
    int error, handle;
    struct hostent *host;
    struct sockaddr_in server;

    host = gethostbyname(host_addr);
    handle = socket(AF_INET, SOCK_STREAM, 0);
    if(handle < 0) {
        std::stringstream buf;
        buf << "socket: " << get_system_error();
        throw ConnectionError(buf.str());
    } else {
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server.sin_zero), 8);

        error = connect(handle,(struct sockaddr *)&server,
                        sizeof(struct sockaddr));
        if(error == -1) {
            std::stringstream buf;
            buf << "Cannot connect to " << host_addr << ":" << port;
            throw ConnectionError(buf.str());
        }
    }

    return handle;
}



void Connection::set_connection(std::string host, int port) {

    socket = tcp_connect(host.c_str(), port);
    if(socket == 0) {
        std::stringstream buf;
        buf << "Cannot connect to " << host << ":" << port;
        throw ConnectionError(buf.str());
    }

    ssl_handle = NULL;
    ssl_context = NULL;

    ssl_context = SSL_CTX_new(SSLv23_client_method());
    if(ssl_context == NULL) {
        close(socket);
        throw ConnectionError(get_ssl_error());
    }

    ssl_handle = SSL_new(ssl_context);
    if(ssl_handle == NULL) {
        close(socket);
        SSL_CTX_free(ssl_context);
        throw ConnectionError(get_ssl_error());
    }

    if(!SSL_set_fd(ssl_handle, socket)) {
        close(socket);
        SSL_CTX_free(ssl_context);
        SSL_shutdown(ssl_handle);
        SSL_free(ssl_handle);
        throw ConnectionError(get_ssl_error());
    }

    if(SSL_connect(ssl_handle) != 1) {
        close(socket);
        SSL_CTX_free(ssl_context);
        SSL_shutdown(ssl_handle);
        SSL_free(ssl_handle);
        throw ConnectionError(get_ssl_error());
    }
}


std::string Connection::receive() {
    const int READ_SIZE = 1024;
    char tmp[READ_SIZE];
    std::stringstream buf;

    while(true) {
        int received = SSL_read(ssl_handle, tmp, READ_SIZE);
        if(received < READ_SIZE) {
            if(tmp[received - 1] != '\0') {
                tmp[received] = '\0';
            }
            buf << tmp;
        } else {
            buf << std::string(tmp, READ_SIZE);
        }

        if(received < READ_SIZE) {
            break;
        }
    }

    return buf.str();
}


void Connection::send(std::string data) {
    if(data.length() % 2 == 0) {
        data += '\0';
    }
    if(SSL_write(ssl_handle, data.c_str(), data.length()) != data.length()) {
        throw ConnectionError("Failed to send");
    }
}
