#ifndef CONFPARSER_H
#define CONFPARSER_H

#include <yaml.h>

#include "maplib.h"

map_t *config_parse(FILE *stream);


typedef enum {
    TYPE_TO_NEVER_USE = 0, TYPE_BIT, TYPE_INTEGER, TYPE_FLOAT
} types_t;


typedef struct {
    types_t ret_type;
    map_t *params;
} rule_t;

#endif
