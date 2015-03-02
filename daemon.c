#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <bsd/libutil.h>
#include <sys/syslog.h>

#include "confparser.h"
#include "targetdevice.h"
#include "binder.h"
#include "maplib.h"
#include "network.h"
#include "processors.h"


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


#define BUF_LEN 512
char *add_str(char *op1, char *op2) {
    char *res;
    if(op1 == NULL) {
        res = strdup(op2);
    } else {
        res = (char*)malloc(strlen(op1) + strlen(op2) + 1);
        res[0] = '\0';
        if(res != NULL) {
            strcpy(res, op1);
            strcat(res, op2);
        }
    }
    if(res == NULL) {
        perror("");
        exit(EXIT_FAILURE);
    }
    return res;
}


map_t *bind_handlers(map_t *rules) {
    map_t *handler_map;
    map_iter_t *rules_iter;
    map_item_t *rules_item;
    char buf[BUF_LEN], *output;

    // Binding
    handler_map = map_create();
    handler_bind(handler_map, "is-connected", is_connected, rules);
    handler_bind(handler_map, "adc-get", adc_get, rules);
    handler_bind(handler_map, "read-all", read_all, rules);

    rules_iter = map_iter(rules);
    while(rules_item = map_iter_next(rules_iter), rules_item != NULL) {
        fprintf(stderr, "%s is not bind\n", rules_item->key);
        free(rules_item);
    }
    free(rules_iter);
    if(map_len(rules) != 0) {
        exit(EXIT_FAILURE);
    }
    return handler_map;
}


void do_network_work(connection_rules_t *conn_rules, map_t *handlers, int serial) {
    ssl_connection_t *conn;
    char *response;
    processored_request_t reqs;
    request_t *req;
    call_handler_result_t *res;

    map_iter_t *iter;
    map_item_t *item;
    char *output;
    char buf[BUF_LEN];

    conn = ssl_connect(conn_rules);
    if(conn == NULL) {
        syslog(LOG_INFO, "Cannot connect to %s:%d",
               conn_rules->host, conn_rules->port);
        return;
    }

    snprintf(buf, BUF_LEN - 1, "ready:%s", conn_rules->identity);
    ssl_write(conn, buf, strlen(buf));
    response = ssl_read(conn);
    reqs = requested_data_parse(response);
    output = NULL;
    if(reqs.status != PROCESSING_REQUEST_SUCCESS) {
        syslog(LOG_INFO, "Wrong request: %s", reqs.message);
        snprintf(buf, 511, ":invalid request:%s\n", reqs.message);
        output = add_str(output, buf);
    }

    iter = map_iter(reqs.value);
    while(item = map_iter_next(iter), item != NULL) {
        req = item->value;
        res = handler_call(handlers, req, serial);
        switch(res->status) {
        case CALL_STATUS_SUCCESS:
            snprintf(buf, 511, "%s:success:%s\n", item->key, res->value);
            break;
        case CALL_STATUS_ERROR:
            snprintf(buf, 511, "%s:invalid request:%s\n", item->key, res->message);
            free(res->message);
            break;
        case CALL_STATUS_INTERNAL_ERROR:
            snprintf(buf, 511, "%s:error:\n", item->key);
            break;
        }
        output = add_str(output, buf);
        free_request_memory(req);
        free(res);
    }
    map_free(reqs.value);

    if(reqs.message != NULL) {
        free(reqs.message);
    }
    ssl_write(conn, output, strlen(output) - 1);
}


int main(int argc, char **argv) {
    config_t *config;
    FILE *config_fp;
    char *config_file_name, *pidfile, *data;
    pid_t otherpid, childpid;
    struct pidfh *pfh;

    map_t *handler_map;
    call_handler_result_t *res;
    int i, do_not_daemonize;
    int serial;

    do_not_daemonize = 0;
    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "--nodaemon") == 0) {
            do_not_daemonize = 1;
        }
    }

    if(argc > 2) {
        config_file_name = argv[0];
    } else {
        config_file_name = "conf/targetdevice.yaml";
    }
    config_fp = fopen(config_file_name, "r");
    if(config_fp == NULL) {
        perror("Cannot open config file");
        return -1;
    }
    config = config_parse(config_fp);

    handler_map = bind_handlers(config->rules);

    serial = port_open(config->daemon->serial);
    if(serial == 0) {
        exit(EXIT_FAILURE);
    }

    if(0 /*!do_not_daemonize*/) {
        pidfile = config->daemon->pidfile;
        pfh = pidfile_open(pidfile, 0600, &otherpid);
        if(pfh == NULL) {
            if(errno == EEXIST) {
                errx(EXIT_FAILURE, "Daemon already running, pidf: %jd.",
                     (intmax_t)otherpid);
            }
            warn("Cannot open or create pidfile");
        }

        if(daemon(0, 0) == -1) {
            warn("Cannot daemonize");
            pidfile_remove(pfh);
            exit(EXIT_FAILURE);
        }

        pidfile_write(pfh);

        /* Do work. */
        childpid = fork();
        switch(childpid) {
        case -1:
            syslog(LOG_ERR, "Cannot fork(): %s.", strerror(errno));
            return -1;
            break;
        case 0:
            pidfile_write(pfh);
            break;
        default:
            syslog(LOG_INFO, "Child %jd started.", (intmax_t)childpid);
            return 0;
            break;
        }
    }


    while(1) {
        do_network_work(config->connection, handler_map, serial);
        sleep(5);
    }

    close(serial);

    return 0;
}
