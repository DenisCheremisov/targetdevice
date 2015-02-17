#include <stdio.h>
#include <assert.h>

#include "confparser.h"


typedef enum {
    ON_RULE_WAIT,
    ON_RULE_NAME,
    ON_RULE_BODY_START,
    ON_RULE_BODY,
    ON_HANDLER_NAME,
    ON_RETURN_TYPE,
    ON_PARAMS_READ,
    ON_PARAM_NAME,
    ON_PARAM_TYPE,
} status_t;


void something_went_wrong_named(yaml_event_t *event, char *message) {
    fprintf(stderr, "%s: (%d,%d)-(%d,%d)\n",
            message,
            (int)event->start_mark.line + 1,
            (int)event->start_mark.column,
            (int)event->end_mark.line + 1,
            (int)event->end_mark.column);
    exit(EXIT_FAILURE);
}


void something_went_wrong_stated(yaml_event_t *event, status_t status) {
    char *message;
    switch(status) {
    case ON_RETURN_TYPE:
        message = "Expected return type";
        break;
    case ON_PARAMS_READ:
        message = "Expected parameters";
        break;
    case ON_PARAM_TYPE:
        message = "Expected parameter type";
        break;
    case ON_RULE_BODY_START:
    case ON_RULE_BODY:
        message = "Expected rule definition";
        break;
    }
    something_went_wrong_named(event, "Parsing error");
}


void definition_is_ok(int handler_is_ok, int ret_type_is_ok, yaml_event_t *event) {
    if(!(handler_is_ok && ret_type_is_ok)) {
        something_went_wrong_named(event, "Incomplete rule");
    }
}


void error_raise() {
    perror("System error");
    exit(EXIT_FAILURE);
}


types_t type_decoded(yaml_event_t *event) {
    char *value;
    value = event->data.scalar.value;
    if(strcmp(value, "bit") == 0) {
        return TYPE_BIT;
    } else if(strcmp(value, "integer") == 0) {
        return TYPE_INTEGER;
    } else if(strcmp(value, "float") == 0) {
        return TYPE_FLOAT;
    }
    fprintf(stderr, "%s - ", value);
    something_went_wrong_named(event, "no such type");
}


map_t* config_parse(FILE *file_stream) {
    yaml_parser_t parser;
    yaml_event_t  event;
    status_t status;
    map_t *result, *params;
    rule_t *rule;
    char *rule_name, *param_name, *value, *tmp;
    int handler_filled, ret_type_filled;
    int params_filled;

    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        fputs("Failed to initialize parser!\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, file_stream);

    status = ON_RULE_WAIT;
    result = map_create();
    do {
        if(!yaml_parser_parse(&parser, &event)) {
            printf("Parser error %d\n", parser.error);
            exit(EXIT_FAILURE);
        }

        switch(event.type) {
        case YAML_NO_EVENT: break;
        case YAML_STREAM_START_EVENT: break;
        case YAML_STREAM_END_EVENT: break;

        case YAML_DOCUMENT_START_EVENT:
            status = ON_RULE_WAIT;
            result = map_create();
            break;

        case YAML_DOCUMENT_END_EVENT:
            if(status != ON_RULE_WAIT) {
                something_went_wrong_stated(&event, status);
            }
            break;

        case YAML_SEQUENCE_START_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            something_went_wrong_named(&event, "Sequences are not supported");
            break;

        case YAML_MAPPING_START_EVENT:
            switch(status) {
            case ON_RULE_WAIT:
                status = ON_RULE_NAME;
                handler_filled = 0;
                ret_type_filled = 0;
                break;
            case ON_RULE_BODY_START:
                status = ON_RULE_BODY;
                break;
            case ON_PARAMS_READ:
                status = ON_PARAM_NAME;
                break;
            default:
                something_went_wrong_stated(&event, status);
            }
            break;

        case YAML_MAPPING_END_EVENT:
            switch(status) {
            case ON_PARAM_NAME:
                if(params_filled == 0) {
                    something_went_wrong_named(&event, "Parameters expected");
                }
                rule->params = params_filled?params:NULL;
                status = ON_RULE_BODY;
                break;
            case ON_RULE_BODY:
                definition_is_ok(handler_filled, ret_type_filled, &event);
                map_set(result, rule_name, rule);
                status = ON_RULE_NAME;
                break;
            case ON_RULE_NAME:
                status = ON_RULE_WAIT;
                break;
            default:
                something_went_wrong_stated(&event, status);
            }
            break;

        case YAML_ALIAS_EVENT:
            something_went_wrong_named(&event, "Aliases are not supported");
            break;

        case YAML_SCALAR_EVENT:
            value = event.data.scalar.value;
            switch(status) {
            case ON_RULE_NAME:
                rule_name = strdup(event.data.scalar.value);
                status = ON_RULE_BODY_START;
                rule = (rule_t*)malloc(sizeof(rule_t));
                params = map_create();
                params_filled = 0;
                break;
            case ON_RULE_BODY:
                if(strcmp(value, "handler") == 0) {
                    status = ON_HANDLER_NAME;
                } else if(strcmp(value, "return") == 0) {
                    status = ON_RETURN_TYPE;
                } else if(strcmp(value, "params") == 0) {
                    status = ON_PARAMS_READ;
                } else {
                    fprintf(stderr, "%s - ", value);
                    something_went_wrong_named(&event, "wrong value for rule");
                }
                break;
            case ON_HANDLER_NAME:
                rule->handler = strdup(value);
                if(!rule->handler) {
                    error_raise();
                }
                status = ON_RULE_BODY;
                handler_filled = 1;
                break;
            case ON_RETURN_TYPE:
                rule->ret_type = type_decoded(&event);
                ret_type_filled = 1;
                status = ON_RULE_BODY;
                break;
            case ON_PARAM_NAME:
                param_name = strdup(value);
                status = ON_PARAM_TYPE;
                break;
            case ON_PARAM_TYPE:
                map_set(params, param_name, (int*)type_decoded(&event));
                status = ON_PARAM_NAME;
                params_filled = 1;
            }
        }

        if(event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }
    } while(event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);


    /* Cleanup */
    yaml_parser_delete(&parser);
    return result;

}
