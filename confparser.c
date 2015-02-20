#include <stdio.h>
#include <assert.h>

#include "confparser.h"


void something_went_wrong(yaml_event_t *event, char *message) {
    fprintf(stderr, "%s: (%d,%d)-(%d,%d)\n",
            message,
            (int)event->start_mark.line + 1,
            (int)event->start_mark.column,
            (int)event->end_mark.line + 1,
            (int)event->end_mark.column);
    exit(EXIT_FAILURE);
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
    something_went_wrong(event, "no such type");
}


void lexem_proceed(yaml_parser_t *parser, yaml_event_t *event) {
    if(!yaml_parser_parse(parser, event)) {
        fprintf(stderr, "Parser error %d\n", parser->error);
        exit(EXIT_FAILURE);
    }
}

#define LEXEM_TYPE_CHECK(event, type_name, message) \
    if(event.type != type_name) something_went_wrong(&event, message);


map_t *parse_rule_params(yaml_parser_t *parser) {
    yaml_event_t event;
    map_t *result;
    int counter;
    char *name;

    result = map_create();
    lexem_proceed(parser, &event);
    LEXEM_TYPE_CHECK(event, YAML_MAPPING_START_EVENT, "Parameters mapping expected");
    counter = 0;
    while(1) {
        lexem_proceed(parser, &event);
        if(event.type == YAML_MAPPING_END_EVENT) {
            break;
        }
        LEXEM_TYPE_CHECK(event, YAML_SCALAR_EVENT, "Parameter name expected");
        name = strdup(event.data.scalar.value);
        if(map_has(result, name)) {
            something_went_wrong(&event, "Duplicate parameter");
        }
        lexem_proceed(parser, &event);
        LEXEM_TYPE_CHECK(event, YAML_SCALAR_EVENT, "Parameter value expected");
        map_set(result, name, (int*)type_decoded(&event));
        counter++;
    }
    if(counter == 0) {
        something_went_wrong(&event, "No parameter defined");
    }
    return result;
}


rule_t *parse_rule(yaml_parser_t *parser) {
    yaml_event_t event;
    rule_t *result;
    int return_defined, params_defined;
    char *value;

    result = (rule_t*)malloc(sizeof(rule_t));
    if(result == NULL) {
        error_raise();
    }

    lexem_proceed(parser, &event);
    LEXEM_TYPE_CHECK(event, YAML_MAPPING_START_EVENT, "Rule definition expected");

    return_defined = params_defined = 0;
    result->params = NULL;
    while(1) {
        lexem_proceed(parser, &event);
        if(event.type == YAML_MAPPING_END_EVENT) {
            break;
        }
        LEXEM_TYPE_CHECK(event, YAML_SCALAR_EVENT, "Syntax error, rule definition expected");
        value = event.data.scalar.value;
        if(strcmp(value, "params") == 0) {
            if(params_defined) {
                something_went_wrong(&event, "Duplicate params definiton");
            }
            result->params = parse_rule_params(parser);
            params_defined = 1;
        } else if(strcmp(value, "return") == 0) {
            if(return_defined) {
                something_went_wrong(&event, "Duplicate return type definition");
            }
            lexem_proceed(parser, &event);
            LEXEM_TYPE_CHECK(event, YAML_SCALAR_EVENT, "Return type expected");
            result->ret_type = type_decoded(&event);
            return_defined = 1;
        } else {
            something_went_wrong(&event, "Wrong identifier, only `return` and `params` are allowed");
        }
    }
    if(return_defined == 0) {
        something_went_wrong(&event, "Return type not defined");
    }
    return result;
}


map_t *parse_rules_body(yaml_parser_t *parser) {
    yaml_event_t event;
    map_t *result;
    rule_t *rule;
    int counter;
    char *value;

    result = map_create();
    if(result == NULL) {
        error_raise();
    }

    lexem_proceed(parser, &event);
    LEXEM_TYPE_CHECK(event, YAML_MAPPING_START_EVENT, "Rules mapping expected");

    counter = 0;
    while(1) {
        lexem_proceed(parser, &event);
        if(event.type == YAML_MAPPING_END_EVENT) {
            break;
        }
        LEXEM_TYPE_CHECK(event, YAML_SCALAR_EVENT, "Rule name expected");
        value = strdup(event.data.scalar.value);
        if(map_has(result, value)) {
            something_went_wrong(&event, "Duplicate rule");
        }
        rule = parse_rule(parser);
        map_set(result, strdup(value), rule);
        counter++;
    }
    if(counter == 0) {
        something_went_wrong(&event, "No rule defined");
    }
    return result;
}


char* read_scalar(yaml_parser_t *parser) {
    yaml_event_t event;
    lexem_proceed(parser, &event);
    if(event.type != YAML_SCALAR_EVENT) {
        return NULL;
    }
    return strdup(event.data.scalar.value);
}


connection_t *parse_connection_body(yaml_parser_t *parser) {
    yaml_event_t event;
    connection_t *result;
    char *value, *num, *tmp;
    int host_defined,
        port_defined,
        certificate_defined,
        identity_defined;
    int number;

    result = (connection_t*)malloc(sizeof(connection_t));
    if(result == NULL) {
        error_raise();
    }

    lexem_proceed(parser, &event);
    LEXEM_TYPE_CHECK(event, YAML_MAPPING_START_EVENT, "Connection mapping expected");

    host_defined = port_defined = certificate_defined = identity_defined = 0;
    while(1) {
        lexem_proceed(parser, &event);
        if(event.type == YAML_MAPPING_END_EVENT) {
            break;
        }
        LEXEM_TYPE_CHECK(event, YAML_SCALAR_EVENT, "Connection mapping expected");
        value = event.data.scalar.value;
        if(strcmp(value, "host") == 0) {
            if(host_defined) {
                something_went_wrong(&event, "Duplicate host definition");
            }
            result->host = read_scalar(parser);
            if(result->host == NULL) {
                something_went_wrong(&event, "Server host address expected");
            }
            host_defined = 1;
        } else if(strcmp(value, "port") == 0) {
            if(port_defined) {
                something_went_wrong(&event, "Duplicate port definition");
            }
            num = read_scalar(parser);
            if(num == NULL) {
                something_went_wrong(&event, "Server port expected");
            }
            number = strtol(num, &tmp, 10);
            if((tmp != NULL && *tmp != '\0')|| (unsigned int)number > 0xffff) {
                something_went_wrong(&event, "Wrong port number");
            }
            result->port = number;
            port_defined = 1;
        } else if(strcmp(value, "certificate") == 0) {
            if(certificate_defined) {
                something_went_wrong(&event, "Duplicate certificate definition");
            }
            result->certificate = read_scalar(parser);
            if(result->certificate == NULL) {
                something_went_wrong(&event, "Certificate file path expected");
            }
            certificate_defined = 1;
        } else if(strcmp(value, "identity") == 0) {
            if(identity_defined) {
                something_went_wrong(&event, "Duplicate identity definition");
            }
            result->identity = read_scalar(parser);
            if(result->identity == NULL) {
                something_went_wrong(&event, "Identity string expected");
            }
            identity_defined = 1;
        } else {
            something_went_wrong(&event, "Wrong identifier");
        }
    }
    if(!host_defined) something_went_wrong(&event, "Host not defined");
    if(!port_defined) something_went_wrong(&event, "Port not defined");
    if(!certificate_defined) something_went_wrong(&event, "Certificate file path not defined");
    if(!identity_defined) something_went_wrong(&event, "Identity string is not defined");
    return result;
}


config_t *parse_all_config(yaml_parser_t *parser) {
    yaml_event_t event;
    config_t *result;
    char *value;
    int rules_defined, connection_defined;

    result = (config_t*)malloc(sizeof(config_t));
    if(result == NULL) {
        error_raise();
    }

    lexem_proceed(parser, &event);
    LEXEM_TYPE_CHECK(event, YAML_MAPPING_START_EVENT, "Rules mapping expected");

    rules_defined = connection_defined = 0;
    while(1) {
        lexem_proceed(parser, &event);
        if(event.type == YAML_MAPPING_END_EVENT) {
            break;
        }
        LEXEM_TYPE_CHECK(event, YAML_SCALAR_EVENT, "Section name expected");
        value = strdup(event.data.scalar.value);
        if(strcmp(value, "connection") == 0) {
            if(connection_defined) {
                something_went_wrong(&event, "Duplicate connection definition");
            }
            result->connection = parse_connection_body(parser);
            connection_defined = 1;
        } else if(strcmp(value, "rules") == 0) {
            if(rules_defined) {
                something_went_wrong(&event, "Duplicate rules definition");
            }
            result->rules = parse_rules_body(parser);
            rules_defined = 1;
        }
    }
    if(!connection_defined) something_went_wrong(&event, "Connection is not defined");
    if(!rules_defined) something_went_wrong(&event, "Rules are not defined");
    return result;
}


config_t *config_parse(FILE *file_stream) {
    yaml_parser_t parser;
    yaml_event_t  event;
    config_t *result;

    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        fputs("Failed to initialize parser!\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, file_stream);

    result = NULL;
    do {
        lexem_proceed(&parser, &event);

        switch(event.type) {
        case YAML_NO_EVENT: break;
        case YAML_STREAM_START_EVENT: break;
        case YAML_STREAM_END_EVENT: break;

        case YAML_DOCUMENT_START_EVENT:;
            result = parse_all_config(&parser);
            break;

        case YAML_DOCUMENT_END_EVENT: break;

        case YAML_SEQUENCE_START_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            something_went_wrong(&event, "Sequences are not supported");
            break;

        case YAML_MAPPING_START_EVENT: break;
        case YAML_MAPPING_END_EVENT: break;
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
