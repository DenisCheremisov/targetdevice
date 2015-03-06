#include <sstream>

#include "confparser.hpp"

using namespace std;


ParserError::ParserError(yaml_event_t &event, string message) {
    stringstream buf;
    buf << "Error at (" << event.start_mark.line + 1 << "," << event.start_mark.column <<
        ")-(" << event.end_mark.line + 1 << "," << event.end_mark.column << "): " << message;

    message = buf.str();
}


ParserError::ParserError(ScalarElement &token, string message) {
    stringstream buf;
    buf << "Error at (" << token.start_pos().first  << "," << token.start_pos().second <<
        ")-(" << token.end_pos().first << "," << token.end_pos().second << "): " <<
        token << " - " << message;

    message = buf.str();
}


void token_proceed(yaml_parser_t &parser, yaml_event_t &event) {
    yaml_event_t new_event;
    if(!yaml_parser_parse(&parser, &new_event)) {
        throw ParserError(event, "parsing error after this token");
    }
    event = new_event;
}


void token_type_match(yaml_event_t &event, yaml_event_type_e type_name, std::string message) {
    if(event.type != type_name) {
        throw ParserError(event, message);
    }
}


ParsedElement *parse_all_config(yaml_parser_t &parser) {
    yaml_event_t event;
    MapElement *result;
    ParsedElement *value;

    token_proceed(parser, event);
    switch(event.type) {
    case YAML_SCALAR_EVENT:
        return new ScalarElement(event);
        break;
    case YAML_MAPPING_START_EVENT:
        result = new MapElement;
        break;
    default:
        throw UnsupportedConstructionError(event, "Only mappings and strings are supported");
    }

    while(true) {
        token_proceed(parser, event);
        if(event.type == YAML_MAPPING_END_EVENT) {
            break;
        }
        token_type_match(event, YAML_SCALAR_EVENT, "Section name expected");
        (*result)[ScalarElement(event)] = parse_all_config(parser);
    }

    return result;
}


MapElement *raw_conf_parse(FILE *fp) {
    yaml_parser_t parser;
    yaml_event_t  event;
    MapElement *result;

    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        fputs("Failed to initialize parser!\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, fp);

    result = NULL;
    do {
        token_proceed(parser, event);

        switch(event.type) {
        case YAML_NO_EVENT: break;
        case YAML_STREAM_START_EVENT: break;
        case YAML_STREAM_END_EVENT: break;

        case YAML_DOCUMENT_START_EVENT:;
            result = dynamic_cast<MapElement*>(parse_all_config(parser));
            if(result == (MapElement*)NULL) {
                throw ParserError(event, "Must be a mapping");
            }
            break;

        case YAML_DOCUMENT_END_EVENT: break;

        case YAML_SEQUENCE_START_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            UnsupportedConstructionError(event, "Sequences are not allowed");
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
