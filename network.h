#ifndef _NETWORK_H_DEFINED_
#define _NETWORK_H_DEFINED_

#include <openssl/ssl.h>

#include "confparser.h"


typedef struct {
    int socket;
    SSL *ssl_handle;
    SSL_CTX *ssl_context;
} ssl_connection_t;


ssl_connection_t *ssl_connect (connection_t *config);
void ssl_disconnect(ssl_connection_t *conn);
char *ssl_read(ssl_connection_t *conn);
void ssl_write(ssl_connection_t *conn, char *text, int len);


#endif
