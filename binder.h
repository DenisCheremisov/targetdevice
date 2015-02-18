#ifndef _BINDER_H_
#define _BINDER_H_

#include "confparser.h"
#include "maplib.h"


typedef char* (*handler_t)(map_t *params);


typedef struct {
    map_t *params;
    char *name;
    types_t ret_type;
    handler_t handler;
} handler_data_t;


void handler_bind(map_t *handler_map, char *handler_name, handler_t handler,
                  map_t *rules);


#endif
