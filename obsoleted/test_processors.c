#include <stdio.h>
#include <assert.h>

#include "processors.h"




int main() {
    processored_request_t reqs;
    request_t *req;
    char *request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n";

    puts("Testing success request");
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status == PROCESSING_REQUEST_SUCCESS);
    assert(map_has(reqs.value, "01"));
    assert(map_has(reqs.value, "02"));
    assert(map_has(reqs.value, "03"));
    req = (request_t*)map_get(reqs.value, "01");
    assert(strcmp(req->name, "adc-get") == 0);
    assert(map_len(req->params) == 1);
    assert(strcmp((char*)map_get(req->params, "channel"), "12") == 0);
    req = (request_t*)map_get(reqs.value, "02");
    assert(strcmp(req->name, "line-set") == 0);
    assert(map_len(req->params) == 2);
    assert(strcmp((char*)map_get(req->params, "lineno"), "0") == 0);
    assert(strcmp((char*)map_get(req->params, "value"), "1") == 0);
    req = (request_t*)map_get(reqs.value, "03");
    assert(strcmp(req->name, "is-connected") == 0);
    assert(map_len(req->params) == 0);
    puts("Test OK\n");

    puts("Testing request_id error");
    request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n"
        "abyr";
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status != PROCESSING_REQUEST_SUCCESS);
    puts(reqs.message);
    puts("Test OK\n");

    puts("Testing name error");
    request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n"
        "abyr:";
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status != PROCESSING_REQUEST_SUCCESS);
    puts(reqs.message);
    puts("Test OK\n");

    puts("Testing parameter error");
    request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n"
        "abyr:valg:";
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status != PROCESSING_REQUEST_SUCCESS);
    puts(reqs.message);
    puts("Test OK\n");

    puts("Testing parameter error");
    request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n"
        "abyr:valg:a";
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status != PROCESSING_REQUEST_SUCCESS);
    puts(reqs.message);
    puts("Test OK\n");

    puts("Testing parameter error");
    request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n"
        "abyr:valg:=b";
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status != PROCESSING_REQUEST_SUCCESS);
    puts(reqs.message);
    puts("Test OK\n");

    puts("Testing parameter error");
    request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n"
        "abyr:valg:a=";
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status != PROCESSING_REQUEST_SUCCESS);
    puts(reqs.message);
    puts("Test OK\n");

    puts("Testing parameter success");
    request =
        "01:adc-get:channel=12\n"
        "02:line-set:lineno=0:value=1\n"
        "03:is-connected\n"
        "abyr:valg:a=b";
    reqs = requested_data_parse(strdup(request));
    assert(reqs.status == PROCESSING_REQUEST_SUCCESS);
    assert(map_has(reqs.value, "01"));
    assert(map_has(reqs.value, "02"));
    assert(map_has(reqs.value, "03"));
    assert(map_has(reqs.value, "abyr"));
    req = (request_t*)map_get(reqs.value, "01");
    assert(strcmp(req->name, "adc-get") == 0);
    assert(map_len(req->params) == 1);
    assert(strcmp((char*)map_get(req->params, "channel"), "12") == 0);
    req = (request_t*)map_get(reqs.value, "02");
    assert(strcmp(req->name, "line-set") == 0);
    assert(map_len(req->params) == 2);
    assert(strcmp((char*)map_get(req->params, "lineno"), "0") == 0);
    assert(strcmp((char*)map_get(req->params, "value"), "1") == 0);
    req = (request_t*)map_get(reqs.value, "03");
    assert(strcmp(req->name, "is-connected") == 0);
    assert(map_len(req->params) == 0);
    req = (request_t*)map_get(reqs.value, "abyr");
    assert(strcmp(req->name, "valg") == 0);
    assert(map_len(req->params) == 1);
    assert(strcmp((char*)map_get(req->params, "a"), "b") == 0);
    puts("Test OK\n");
}
