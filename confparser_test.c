#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "confparser.h"

int test1(void) {
    map_t *rules;
    connection_rules_t *connection;
    config_t *config;
    daemon_rules_t *daemon;
    rule_t *rule;
    char *param;
    FILE *fh = fopen("conf/test.yaml", "r");
    if(fh == NULL)
        fputs("Failed to open file!\n", stderr);
    config = config_parse(fh);
    rules = config->rules;
    fclose(fh);

    rule = map_get(rules, "is-connected");

    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    map_len(rule->params);
    assert(map_len(rule->params) == 0);

    rule = map_get(rules, "relay-set");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    param = map_get(rule->params, "port");
    assert((types_t)param == TYPE_INTEGER);
    param = map_get(rule->params, "value");
    assert((types_t)param == TYPE_BIT);
    assert(map_len(rule->params) == 2);

    rule = map_get(rules, "line-get");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    param = map_get(rule->params, "lineno");
    assert((types_t)param == TYPE_INTEGER);
    assert(map_len(rule->params) == 1);

    rule = map_get(rules, "line-set");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_BIT);
    param = map_get(rule->params, "lineno");
    assert((types_t)param == TYPE_INTEGER);
    param = map_get(rule->params, "value");
    assert((types_t)param == TYPE_BIT);
    assert(map_len(rule->params) == 2);

    rule = map_get(rules, "adc-get");
    assert(rule != NULL);
    assert((types_t)rule->ret_type == TYPE_FLOAT);
    param = map_get(rule->params, "channel");
    assert((types_t)param == TYPE_INTEGER);
    assert(map_len(rule->params) == 1);

    assert(map_len(rules) == 5);


    // Connection
    connection = config->connection;
    assert(strcmp(connection->host, "localhost") == 0);
    assert(connection->port == 10023);
    assert(strcmp(connection->identity, "client_001") == 0);

    // Daemon rules
    daemon = config->daemon;
    assert(strcmp(daemon->logfile, "/var/log/targetdevice.log") == 0);
    assert(strcmp(daemon->pidfile, "/var/pids/targetdevice.pid") == 0);

    printf("conf/test.yaml config parsed successfuly\n");

    return 0;
}


int main(void)
{
    test1();
    return 0;
}
