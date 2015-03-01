#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "confparser.h"
#include "binder.h"


work_result_t *proper_handler(map_t *params, int serial) {
    return NULL;
}


int test1() {
    map_t *conf, *handler_map;
    FILE *conf_file;
    handler_data_t *semantics;
    types_t type;

    conf_file = fopen("conf/test.yaml", "r");
    if(conf_file == NULL) {
        return -1;
    }
    conf = config_parse(conf_file)->rules;
    fclose(conf_file);
    assert(map_len(conf) == 6);

    handler_map = map_create();
    if(handler_map == NULL) {
        return -1;
    }

    handler_bind(handler_map, "line-set", proper_handler, conf);
    assert(map_len(handler_map) == 1);
    assert(map_len(conf) == 5);

    handler_bind(handler_map, "adc-get", proper_handler, conf);
    assert(map_len(handler_map) == 2);
    assert(map_len(conf) == 4);

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

    return 0;
}


work_result_t *real_work(map_t *params, int serial) {
    work_result_t *result;
    double res = 0.0;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    assert(result != NULL);
    RESULT_SUCCESS(result, res, double);
    return result;
}


work_result_t *real_error(map_t *params, int serial) {
    work_result_t *result;
    double res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    assert(result != NULL);
    RESULT_ERROR(result);
    return result;
}


work_result_t *string_work(map_t *params, int serial) {
    work_result_t *result;
    char *res = "010000x01x00100000";
    result = (work_result_t*)malloc(sizeof(work_result_t));
    assert(result != NULL);
    RESULT_SUCCESS_STR(result, res, strlen(res));
    return result;
}


int test2() {
    map_t *conf, *handler_map;
    FILE *conf_file;
    handler_data_t *semantics;
    types_t type;
    call_handler_result_t *res;
    request_t request;

    conf_file = fopen("conf/test.yaml", "r");
    if(conf_file == NULL) {
        return -1;
    }
    conf = config_parse(conf_file)->rules;
    fclose(conf_file);

    handler_map = map_create();
    if(handler_map == NULL) {
        return -1;
    }

    handler_bind(handler_map, "line-set", real_error, conf);
    handler_bind(handler_map, "adc-get", real_work, conf);
    handler_bind(handler_map, "line-get-all", string_work, conf);

    puts("Check on method not available error");
    request.name = "adc-get1";
    request.params = map_create();
    res = handler_call(handler_map, &request, 0);
    assert(res->status == CALL_STATUS_ERROR);
    assert(res->message != NULL);
    printf("%s\n", res->message);
    puts("Check passed");
    free(res);

    puts("\nCheck on params list mismatch");
    request.name = "adc-get";
    res = handler_call(handler_map, &request, 0);
    assert(res->status == CALL_STATUS_ERROR);
    assert(res->message != NULL);
    printf("%s\n", res->message);
    puts("Check passed");
    free(res);

    puts("\nCheck on wrong parameter type");
    map_set(request.params, "channel", strdup("a"));
    res = handler_call(handler_map, &request, 0);
    assert(res->status == CALL_STATUS_ERROR);
    assert(res->message != NULL);
    printf("%s\n", res->message);
    puts("Check passed");
    free(res);

    puts("\nCheck on success");
    map_set(request.params, "channel", strdup("1"));
    res = handler_call(handler_map, &request, 0);
    assert(res->status == CALL_STATUS_SUCCESS);
    assert(res->message == NULL);
    printf("Return: %s\n", res->value);
    puts("Check passed");
    free(res);

    puts("\nCheck on wrong value parameter");
    request.name = "line-set";
    map_pop(request.params, "channel");
    map_set(request.params, "lineno", strdup("1"));
    map_set(request.params, "value", strdup("2"));
    res = handler_call(handler_map, &request, 0);
    assert(res->status == CALL_STATUS_ERROR);
    assert(res->message != NULL);
    printf("%s\n", res->message);
    puts("Check passed");
    free(res);

    puts("\nCheck on work error");
    request.name = "line-set";
    map_pop(request.params, "channel");
    map_set(request.params, "lineno", strdup("1"));
    map_set(request.params, "value", strdup("0"));
    res = handler_call(handler_map, &request, 0);
    assert(res->status == CALL_STATUS_INTERNAL_ERROR);
    assert(res->message == NULL);
    puts("Check passed");
    free(res);

    puts("\nCheck on return type string");
    request.name = "line-get-all";
    map_free(request.params);
    request.params = NULL;
    res = handler_call(handler_map, &request, 0);
    assert(res->status == CALL_STATUS_SUCCESS);
    assert(res->message == NULL);
    printf("Return: %s\n", res->value);
    assert(strcmp(res->value, "010000x01x00100000") == 0);
    puts("Check passed");

    return 0;
}


int main(int argc, char *argv) {
    if(test1() < 0) {
        return -1;
    }
    if(test2() < 0) {
        return -1;
    }
    return 0;
}
