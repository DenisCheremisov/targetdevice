#include <stdio.h>
#include <stdlib.h>

#include "binder.h"


void handler_bind(map_t *handler_map, char *handler_name, handler_t handler,
                  map_t *rules) {
    rule_t *value;
    handler_data_t *binding;

    value = (rule_t*)map_pop(rules, handler_name);
    if(value == NULL) {
        fprintf(stderr, "No such handler in config: %s\n", handler_name);
        free(value);
        exit(EXIT_FAILURE);
    }

    binding = malloc(sizeof(handler_data_t));
    if(!binding) {
        perror("");
        exit(EXIT_FAILURE);
    }

    binding->params = value->params;
    binding->name = strdup(handler_name);
    if(binding->name == NULL) {
        exit(EXIT_FAILURE);
    }
    binding->ret_type = value->ret_type;
    binding->handler = handler;
    free(value);

    map_set(handler_map, handler_name, binding);
}
