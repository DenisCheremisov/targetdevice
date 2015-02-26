#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "processors.h"


typedef enum {
    PARSE_LINE_SUCCESS,
    PARSE_LINE_REQUEST_ID_EXPECTED,
    PARSE_LINE_METHOD_NAME_EXPECTED,
    PARSE_LINE_PARAMETER_ASSIGNMENT_EXPECTED
} parse_line_status_t;


request_t *parse_line(char **request, parse_line_status_t *status) {
    request_t *result;
    map_t *params;
    char *token;
    char *param, *value;

    result = (request_t*)malloc(sizeof(request_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }
    *status = PARSE_LINE_SUCCESS;

    // Get the request id
    token = strsep(request, ":");
    if(*request == NULL) {
        *status = PARSE_LINE_REQUEST_ID_EXPECTED;
        free(result);
        return NULL;
    }
    result->request_id = strdup(token);

    result->params = NULL;
    // Get method name
    token = strsep(request, ":");
    if(strlen(token) < 1) {
        *status = PARSE_LINE_METHOD_NAME_EXPECTED;
        free(result);
        return NULL;
    }
    result->name = strdup(token);
    if(*request == NULL) {
        return result;
    }

    params = map_create();
    // Get parameters
    do {
        token = strsep(request, ":");
        value = token;
        param = strsep(&value, "=");
        if(value == NULL || strlen(param) == 0 || strlen(value) == 0) {
            *request = param;
            *status = PARSE_LINE_PARAMETER_ASSIGNMENT_EXPECTED;
            return NULL;
        }
        map_set(params, param, strdup(value));
    } while(*request != NULL);

    result->params = params;
    return result;
}


void free_request_memory(request_t *req) {
    free(req->request_id);
    free(req->name);
    map_free(req->params);
}


processored_request_t requested_data_parse(char *request) {
    processored_request_t status_res;
    map_t *result;
    char *cursor, *line, *start, *line_copy;
    request_t *req;
    int line_no;
    parse_line_status_t status;
    char message[1024];

    result = map_create();
    line_no = 1;
    status_res.value = result;
    do {
        line = strsep(&request, "\n");
        line_copy = strdup(line);
        start = line;
        req = parse_line(&line, &status);
        if(status != PARSE_LINE_SUCCESS) {
            snprintf(message, 1023,
                     "Parsing request error %d at line %d: %s",
                     status, line_no, line_copy);
            status_res.status = PROCESSING_REQUEST_ERROR;
            status_res.message = message;
            free(line_copy);
            return status_res;
        }
        line_no++;
        free(line_copy);
        map_set(result, req->request_id, req);
    } while(request != NULL && strlen(request) > 0);
    status_res.message = NULL;
    status_res.status = PROCESSING_REQUEST_SUCCESS;
    status_res.value = result;
    return status_res;
}
