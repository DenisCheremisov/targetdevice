#ifndef CONFPARSER_H
#define CONFPARSER_H

#include <yaml.h>

#include "maplib.h"


typedef enum {
    TYPE_TO_NEVER_USE = 0, TYPE_BIT, TYPE_INTEGER, TYPE_FLOAT
} types_t;


typedef struct {
    types_t ret_type;
    map_t *params;
} rule_t;


typedef struct {
    char *host;
    int port;
    char *certificate;
    char *identity;
} connection_t;


typedef struct {
    map_t *rules;
    connection_t *connection;
} config_t;


config_t *config_parse(FILE *stream);

#endif
