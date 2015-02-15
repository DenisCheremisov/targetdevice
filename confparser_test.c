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
    assert(strcmp(rule->handler, "just_ke") == 0);
    assert(strcmp(rule->ret_type, "bool") == 0);
    map_len(rule->params);
    assert(map_len(rule->params) == 0);

    rule = map_get(res, "relay-set");
    assert(rule != NULL);
    assert(strcmp(rule->handler, "relay_set") == 0);
    assert(strcmp(rule->ret_type, "bool") == 0);
    param = map_get(rule->params, "port");
    assert(strcmp(param, "integer") == 0);
    param = map_get(rule->params, "value");
    assert(strcmp(param, "bool") == 0);
    assert(map_len(rule->params) == 2);

    rule = map_get(res, "line-get");
    assert(rule != NULL);
    assert(strcmp(rule->handler, "line_get") == 0);
    assert(strcmp(rule->ret_type, "bool") == 0);
    param = map_get(rule->params, "lineno");
    assert(strcmp(param, "integer") == 0);
    assert(map_len(rule->params) == 1);

    rule = map_get(res, "line-set");
    assert(rule != NULL);
    assert(strcmp(rule->handler, "line_set") == 0);
    assert(strcmp(rule->ret_type, "bool") == 0);
    param = map_get(rule->params, "lineno");
    assert(strcmp(param, "integer") == 0);
    param = map_get(rule->params, "value");
    assert(strcmp(param, "bool") == 0);
    assert(map_len(rule->params) == 2);

    rule = map_get(res, "adc-get");
    assert(rule != NULL);
    assert(strcmp(rule->handler, "adc_get") == 0);
    assert(strcmp(rule->ret_type, "float") == 0);
    param = map_get(rule->params, "channel");
    assert(strcmp(param, "integer") == 0);
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
