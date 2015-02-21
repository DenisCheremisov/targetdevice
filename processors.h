#ifndef _PROCESSORS_H_DEFINED_
#define _PROCESSORS_H_DEFINED_

#include "binder.h"


typedef struct {
    enum {
        PROCESSING_REQUEST_SUCCESS,
        PROCESSING_REQUEST_ERROR
    } status;
    map_t *value;
    char *message;
} processored_request_t;


processored_request_t requested_data_parse(char *request);

#endif
