#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "confparser.h"
#include "binder.h"


char* proper_handler(map_t *params) {
    return "";
}


int main(int argc, char **argv) {
    map_t *conf, *handler_map;
    FILE *conf_file;
    handler_data_t *semantics;
    types_t type;

    conf_file = fopen("conf/test.yaml", "r");
    if(conf_file == NULL) {
        return -1;
    }
    conf = config_parse(conf_file);
    fclose(conf_file);
    assert(map_len(conf) == 5);

    handler_map = map_create();
    if(handler_map == NULL) {
        return -1;
    }

    handler_bind(handler_map, "line-set", proper_handler, conf);
    assert(map_len(handler_map) == 1);
    assert(map_len(conf) == 4);

    handler_bind(handler_map, "adc-get", proper_handler, conf);
    assert(map_len(handler_map) == 2);
    assert(map_len(conf) == 3);

    assert(map_has(handler_map, "line-set"));
    assert(map_has(handler_map, "adc-get"));

    semantics = (handler_data_t*)map_get(handler_map, "line-set");
    assert(strcmp(semantics->name, "line-set") == 0);
    assert(semantics->ret_type == TYPE_BIT);
    assert(map_len(semantics->params) == 2);
    type = (types_t)map_get(semantics->params, "lineno");
    assert(type == TYPE_INTEGER);
    type = (types_t)map_get(semantics->params, "value");
    assert(type == TYPE_BIT);

    semantics = (handler_data_t*)map_get(handler_map, "adc-get");
    assert(strcmp(semantics->name, "adc-get") == 0);
    assert(semantics->ret_type == TYPE_FLOAT);
    assert(map_len(semantics->params) == 1);
    type = (types_t)map_get(semantics->params, "channel");
    assert(type == TYPE_INTEGER);

    assert(check_float("1.23434") == 0);
    assert(check_float("12.1ipo") < 0);
    assert(check_float("a12") < 0);

    assert(check_integer("1.23434") < 0);
    assert(check_integer("12.1ipo") < 0);
    assert(check_integer("a12") < 0);
    assert(check_integer("123") == 0);

    assert(check_bit("1.23434") < 0);
    assert(check_bit("12.1ipo") < 0);
    assert(check_bit("a12") < 0);
    assert(check_bit("123") < 0);
    assert(check_bit("1") == 0);
    assert(check_bit("0") == 0);

    printf("Success!\n");

    return 0;
}
