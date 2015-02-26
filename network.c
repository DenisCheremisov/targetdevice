#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#include "network.h"


int tcp_connect(connection_rules_t *config) {
    int error, handle;
    struct hostent *host;
    struct sockaddr_in server;

    host = gethostbyname(config->host);
    handle = socket(AF_INET, SOCK_STREAM, 0);
    if(handle < 0) {
        perror("Socket");
        handle = 0;
    } else {
        server.sin_family = AF_INET;
        server.sin_port = htons(config->port);
        server.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server.sin_zero), 8);

        error = connect(handle,(struct sockaddr *)&server,
                        sizeof(struct sockaddr));
        if(error == -1) {
            perror("Connect");
            handle = 0;
        }
    }

    return handle;
}


ssl_connection_t *ssl_connect(connection_rules_t *config) {
    ssl_connection_t *conn;

    conn = (ssl_connection_t*)malloc(sizeof(ssl_connection_t));
    conn->ssl_handle = NULL;
    conn->ssl_context = NULL;

    conn->socket = tcp_connect(config);
    if(conn->socket) {
        SSL_load_error_strings();
        SSL_library_init();

        conn->ssl_context = SSL_CTX_new(SSLv23_client_method());
        if(conn->ssl_context == NULL) {
            ERR_print_errors_fp(stderr);
        }

        conn->ssl_handle = SSL_new(conn->ssl_context);
        if(conn->ssl_handle == NULL) {
            ERR_print_errors_fp(stderr);
        }

        if(!SSL_set_fd(conn->ssl_handle, conn->socket)) {
            ERR_print_errors_fp(stderr);
        }

        if(SSL_connect(conn->ssl_handle) != 1) {
            ERR_print_errors_fp(stderr);
        }
    }  else {
        perror("Connect failed");
    }

    return conn;
}


void ssl_disconnect(ssl_connection_t *conn) {
    if(conn->socket) {
        close(conn->socket);
    }
    if(conn->ssl_handle) {
        SSL_shutdown(conn->ssl_handle);
        SSL_free(conn->ssl_handle);
    }
    if(conn->ssl_context) {
        SSL_CTX_free(conn->ssl_context);
    }
    free(conn);
}


char *ssl_read(ssl_connection_t *conn) {
    const int read_size = 1024;
    char *rc = NULL;
    int received, count = 0;
    char buffer[1024];

    if(conn) {
        while(1) {
            if(!rc) {
                rc = (char*)malloc(read_size * sizeof(char) + 1);
                rc[0] = '\0';
            } else {
                rc = (char*)realloc(rc,(count + 1)*read_size*sizeof(char) + 1);
            }

            received = SSL_read(conn->ssl_handle, buffer, read_size);
            buffer[received] = '\0';

            if(received > 0) {
                strcat(rc, buffer);
            }

            if(received < read_size) {
                break;
            }
            count++;
        }
    }

    return rc;
}


void ssl_write(ssl_connection_t *conn, char *data, int len) {
    if (conn) {
        if(SSL_write(conn->ssl_handle, data, len) != len) {
            perror("SSL: failed to write");
        }
    }
}
