#ifndef CONFPARSER_H
#define CONFPARSER_H

#include <yaml.h>

#include "map_lib.h"

map_t *config_parse(FILE *stream);


typedef struct {
    char *handler;
    char *ret_type;
    map_t *params;
} rule_t;

#endif
