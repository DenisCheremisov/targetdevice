#include "getters.h"
#include "targetdevice.h"


work_result_t *is_connected(map_t *params, int serial) {
    work_result_t *result;
    char response[256];

    int boolean;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    if(port_talk(serial, "$KE\r\n", response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(strncmp(response, "#OK", 3) == 0) {
            boolean = 1;
            RESULT_SUCCESS(result, boolean, int);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *adc_get(map_t *params, int serial) {
    work_result_t *result;
    double resp;
    char command[512], response[256];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command) - 1, "$KE,ADC,%s\r\n", (char*)map_get(params, "channel"));
    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(sscanf(response, "#ADC,%d,%d", &_var, &res) == 2) {
            resp = (double)res * 5. / 1023.;
            RESULT_SUCCESS(result, resp, double);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *read_all(map_t *params, int serial) {
    work_result_t *result;
    double resp;
    char command[512], response[256], str[64];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command) - 1, "$KE,RD,ALL\r\n");
    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(sscanf(response, "#RD,%s", str) == 1) {
            RESULT_SUCCESS_STR(result, str, 18);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *read_line(map_t *params, int serial) {
    work_result_t *result;
    char command[512], response[256];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command) - 1, "$KE,RD,%s\r\n", (char*)map_get(params, "lineno"));
    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(sscanf(response, "#RD,%d,%d", &_var, &res) == 2) {
            RESULT_SUCCESS(result, res, int);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *write_line(map_t *params, int serial) {
    work_result_t *result;
    char command[512], response[256];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command) - 1, "$KE,WR,%s,%s\r\n",
             (char*)map_get(params, "lineno"),
             (char*)map_get(params, "value"));
    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(strncmp(response, "#WR,OK", 6) == 0) {
            res = 1;
            RESULT_SUCCESS(result, res, int);
        } else if(strncmp(response, "#WR,WRONGLINE", 13) == 0) {
            res = 0;
            RESULT_SUCCESS(result, res, int);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *relay_set(map_t *params, int serial) {
    work_result_t *result;
    char command[512], response[256];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }


    snprintf(command, sizeof(command) - 1, "$KE,REL,%s,%s\r\n",
             (char*)map_get(params, "relayno"),
             (char*)map_get(params, "value"));
    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(strncmp(response, "#REL,OK", 7) == 0) {
            res = 1;
            RESULT_SUCCESS(result, res, int);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *io_get_mem_all(map_t *params, int serial) {
    work_result_t *result;
    char command[512], response[256], str[64];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    strcpy(command, "$KE,IO,GET,MEM\r\n");
    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(sscanf(response, "#IO,%s", str) == 1) {
            RESULT_SUCCESS_STR(result, str, 18);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *io_get_mem(map_t *params, int serial) {
    work_result_t *result;
    char command[512], response[256], str[64];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command) - 1, "$KE,IO,GET,MEM,%s\r\n",
             (char*)map_get(params, "lineno"));
    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(sscanf(response, "#IO,%d", &res) == 1) {
            RESULT_SUCCESS(result, res, int);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *io_set(map_t *params, int serial) {
    work_result_t *result;
    char command[512], response[256];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command) - 1, "$KE,IO,SET,%s,%s,S\r\n",
             (char*)map_get(params, "lineno"), (char*)map_get(params, "value"));

    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(strncmp(response, "#IO,SET,OK", 10) == 0) {
            res = 1;
            RESULT_SUCCESS(result, res, int);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}


work_result_t *afr_set(map_t *params, int serial) {
    work_result_t *result;
    char command[512], response[256];
    int _var, res;
    result = (work_result_t*)malloc(sizeof(work_result_t));
    if(result == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command) - 1, "$KE,AFR,%s\r\n",
             (char*)map_get(params, "value"));

    if(port_talk(serial, command, response) < 0) {
        RESULT_ERROR(result);
    } else {
        if(strncmp(response, "#AFR,OK", 7) == 0) {
            res = 1;
            RESULT_SUCCESS(result, res, int);
        } else {
            RESULT_ERROR(result);
        }
    }

    return result;
}
