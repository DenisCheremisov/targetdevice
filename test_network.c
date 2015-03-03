#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "confparser.h"
#include "network.h"


int main(int argc, char **argv) {
    ssl_connection_t *conn;
    config_t *conf;
    FILE *fp;
    char *response = NULL;

    fp = fopen("conf/test.yaml", "r");
    conf = config_parse(fp);
    if(conf == NULL) {
        perror("Config parser error");
        exit(EXIT_FAILURE);
    }
    conn = ssl_connect(conf->connection);

    ssl_write(conn, "ready", 5);
    response = ssl_read(conn);
    assert(response != NULL);
    puts(response);

    ssl_write(conn, "not ready", 9);
    response = ssl_read(conn);
    assert(response != NULL);
    puts(response);

    ssl_write(conn, "ready", 5);
    response = ssl_read(conn);
    assert(response != NULL);
    puts(response);

    ssl_disconnect(conn);
}
