#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#include "buffer.h"
#include "c_hashmap-master/hashmap.h"
#include "header_fields.h"

static const uint8_t METHOD_NO_AUTHENTICATION_REQUIRED = 0x00;
static const uint8_t METHOD_NO_ACCEPTABLE_METHODS = 0xFF;

/* Request Methods */
extern char *request_methods[];

/* Protocol Versions */
extern char *protocol_versions[];

typedef map_t header_map_t;

struct http_request
{
    /** request-line **/
    const char *method_token;
    const char *uri;
    const char *protocol_version;

    /** headers **/
    header_map_t header_map;

    /** message body **/
    const char *body;
};

enum parser_state
{
    parser_method,
    parser_uri,
    parser_protocol_version,
    parser_header_fields,
    parser_empty_line,
    parser_message_body,
    parser_done,
    parser_error_unsupported_method,
    parser_error_unsupported_uri,
    parser_error_unsupported_protocol_version,
    parser_error_unsupported_header_fields,
    parser_error_unsupported_empty_line,
    parser_error_unsupported_message_body,
    parser_error_unsupported_version,
};

struct http_parser
{
    struct http_request *request;

    enum parser_state state;

    /* methods remaining to be read */
    uint8_t remaining;
};

void http_parser_init(struct http_parser *p);

/** feed a string to parser. return true if finished  */
enum parser_state http_parser_feed(struct http_parser *p, const char *s);

/** feed a request line  */
enum parser_state http_parser_feed_request_line(struct http_parser *p, char *line);

/** get error message from enum name **/
const char * parse_error(enum parser_state state);

#endif
