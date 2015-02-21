#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

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


int check_float(char *value) {
    int count;
    double dest;
    char *rem;

    rem = strdup(value);
    if(rem == NULL) {
        exit(EXIT_FAILURE);
    }
    count = sscanf(value, "%lf%s", &dest, rem);
    if(count == 1) {
        return 0;
    }
    return -1;
}


int check_integer(char *value) {
    int count;
    int dest;
    char *rem;

    rem = strdup(value);
    if(rem == NULL) {
        exit(EXIT_FAILURE);
    }
    count = sscanf(value, "%d%s", &dest, rem);
    if(count == 1) {
        return 0;
    }
    return -1;
}


int check_bit(char *value) {
    int count;
    int dest;
    char *rem;

    rem = strdup(value);
    if(rem == NULL) {
        exit(EXIT_FAILURE);
    }
    count = sscanf(value, "%d%s", &dest, rem);
    if(count == 1 && -1 < dest && dest < 2) {
        return 0;
    }
    return -1;
}


const int ERROR_BUFFER_LENGTH = 1024;


char* error_buffer(char *msg) {
    char *result;
    result = (char*)malloc(ERROR_BUFFER_LENGTH);
    if(result == NULL) {
        exit(EXIT_FAILURE);
    }
    if(msg != NULL) {
        strncpy(result, msg, ERROR_BUFFER_LENGTH - 1);
    }
    return result;
}


call_handler_result_t* handler_call(map_t *handler_map, request_t *request) {
    call_handler_result_t *result;
    types_t type;
    map_t *iter;
    handler_data_t *handler_data;
    char *value, *arg, *param_value, *message;
    work_result_t *handler_result;
    map_t *call_params;
    void *call_value, *tmp_value;
    int len, tmp_int;
    double tmp_float;
    void *param;

    message = NULL;
    result = (call_handler_result_t*)malloc(sizeof(call_handler_result_t));
    if(result == NULL) {
        exit(EXIT_FAILURE);
    }

    handler_data = map_get(handler_map, request->name);
    if(handler_data == NULL) {
        message = error_buffer(NULL);
        snprintf(message, ERROR_BUFFER_LENGTH - 1, "Method %s is not defined", request->name);
        goto does_not_need_to_clean;
    }
    if(map_len(handler_data->params) != map_len(request->params)) {
        message = error_buffer("Parameters do not satisfy handler definition");
        goto does_not_need_to_clean;
    }


    call_params = map_create();
    for(iter = request->params; iter != NULL && iter->key != NULL; iter = iter->next) {
        param = map_get(handler_data->params, iter->key);
        if(param == NULL) {
            message = error_buffer(NULL);
            snprintf(message, ERROR_BUFFER_LENGTH - 1,
                     "Parameter %s is not supported", iter->key);
            goto need_to_clean;
        }
        param_value = (char*)iter->value;
        switch((types_t)param) {
        case TYPE_FLOAT:
            if(check_float(param_value) != 0) {
                message = error_buffer(NULL);
                snprintf(message, ERROR_BUFFER_LENGTH - 1,
                         "Parameter %s value is not a float, got \"%s\" instead", iter->key,
                         param_value);
                goto need_to_clean;
            }
            len = sizeof(double);
            tmp_float = atof(param_value);
            call_value = (void*)&tmp_float;
            break;
        case TYPE_INTEGER:
            if(check_integer(param_value) != 0) {
                message = error_buffer(NULL);
                snprintf(message, ERROR_BUFFER_LENGTH - 1,
                         "Parameter %s value is not an integer, got \"%s\" instead", iter->key,
                         param_value);
                goto need_to_clean;
            }
            len = sizeof(int);
            tmp_int = atol(param_value);
            call_value = (void*)&tmp_int;
            break;
        case TYPE_BIT:
            if(check_bit(param_value) != 0) {
                message = error_buffer(NULL);
                snprintf(message, ERROR_BUFFER_LENGTH - 1,
                         "Parameter %s value is not a bit (i.e. either 0 or 1), got \"%s\" instead",
                         iter->key, param_value);
                goto need_to_clean;
            }
            len = sizeof(int);
            tmp_int = atol(param_value);
            call_value = (void*)&tmp_int;
            break;
        }
        tmp_value = malloc(len);
        if(tmp_value == NULL) {
            exit(EXIT_FAILURE);
        }
        memcpy(tmp_value, (void*)call_value, len);
        map_set(call_params, iter->key, (void*)tmp_value);
    }

    // Now call for method
    handler_result = handler_data->handler(call_params);
    if(handler_result->status == WORKER_STATUS_ERROR) {
        result->status = CALL_STATUS_INTERNAL_ERROR;
    } else {
        result->status = CALL_STATUS_SUCCESS;
        switch(handler_data->ret_type) {
        case TYPE_FLOAT:
            sprintf(result->value, "%.8lf", *(double*)handler_result->result);
            break;
        case TYPE_INTEGER:
            sprintf(result->value, "%d", *(int*)handler_result->result);
            break;
        case TYPE_BIT:
            sprintf(result->value, "%d", *(int*)handler_result->result);
            break;
        }
    }

    map_free(call_params);
    free(handler_result);
    result->message = NULL;
    return result;

 need_to_clean:
    map_free(call_params);
 does_not_need_to_clean:
    result->status = CALL_STATUS_ERROR;
    result->message = message;
    return result;
}