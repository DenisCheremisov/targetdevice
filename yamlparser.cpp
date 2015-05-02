#include "yamlparser.hpp"


std::ostream& operator<<(std::ostream& buf, const ScalarElement& data) {
    buf << (std::string)data << " (" << data.start_pos().first << "," <<
        data.start_pos().second << " - " <<
        data.end_pos().first << "," << data.end_pos().second << ")";
    return buf;
}


BaseStruct *yaml_parse(YamlParser *parser, BaseStruct *strct) {
    BaseStruct *res;
    while(true) {
        YamlEvent *event = parser->get_event();
        switch(event->type) {
        case YAML_NO_EVENT: break;
        case YAML_STREAM_START_EVENT: break;
        case YAML_STREAM_END_EVENT: break;

        case YAML_DOCUMENT_START_EVENT:;
            res = strct->build(parser);
            break;

        case YAML_DOCUMENT_END_EVENT: break;

        case YAML_SEQUENCE_START_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            YamlParserError(event, "Sequences are not allowed");
            break;

        case YAML_MAPPING_START_EVENT: break;
        case YAML_MAPPING_END_EVENT: break;
        default: ;
        }
        if(event->type == YAML_DOCUMENT_END_EVENT) {
            break;
        }
    }
    return res;
}
