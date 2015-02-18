#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "confparser.h"

int test1(void) {
    map_t *res;
    rule_t *rule;
    char *param;
    FILE *fh = fopen("conf/test.yaml", "r");
    if(fh == NULL)
        fputs("Failed to open file!\n", stderr);
    res = config_parse(fh);
    fclose(fh);

    rule = map_get(res, "is-connected");

    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    map_len(rule->params);
    assert(map_len(rule->params) == 0);

    rule = map_get(res, "relay-set");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    param = map_get(rule->params, "port");
    assert((types_t)param == TYPE_INTEGER);
    param = map_get(rule->params, "value");
    assert((types_t)param == TYPE_BIT);
    assert(map_len(rule->params) == 2);

    rule = map_get(res, "line-get");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    param = map_get(rule->params, "lineno");
    assert((types_t)param == TYPE_INTEGER);
    assert(map_len(rule->params) == 1);

    rule = map_get(res, "line-set");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    param = map_get(rule->params, "lineno");
    assert((types_t)param == TYPE_INTEGER);
    param = map_get(rule->params, "value");
    assert((types_t)param == TYPE_BIT);
    assert(map_len(rule->params) == 2);

    rule = map_get(res, "adc-get");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_FLOAT);
    param = map_get(rule->params, "channel");
    assert((types_t)param == TYPE_INTEGER);
    assert(map_len(rule->params) == 1);

    assert(map_len(res) == 5);

    printf("conf/test.yaml config parsed successfuly\n");

    return 0;
}


int main(void)
{
    test1();
    return 0;
}
