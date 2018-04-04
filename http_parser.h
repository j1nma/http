#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#include "c_hashmap-master/hashmap.h"
#include "header_fields.h"

static const uint8_t METHOD_NO_AUTHENTICATION_REQUIRED = 0x00;
static const uint8_t METHOD_NO_ACCEPTABLE_METHODS = 0xFF;

/* Request Methods */
extern char *request_methods[];

/* Protocol Versions */
extern char *protocol_versions[];

struct http_request
{
    /** request-line **/
    char *method_token;
    char *uri;
    char *protocol_version;

    /** message body **/
    char *body;

    /** headers **/
    map_t *header_map;
};

enum parser_state
{
    parser_request_line,
    parser_header_fields,
    parser_empty_line,
    parser_message_body,

    parser_method,
    parser_uri,
    parser_protocol_version,

    parser_done,

    parser_error_request_line,
    parser_error_header_field,

    parser_error_unsupported_method,
    parser_error_unsupported_uri,
    parser_error_unsupported_protocol_version,
    parser_error_unsupported_header_fields,
    parser_error_unsupported_empty_line,
    parser_error_unsupported_message_body,
    parser_error_unsupported_version
};

struct http_parser
{
    struct http_request *request;

    enum parser_state state;
};

/** initialize parser **/
void http_parser_init(struct http_parser *parser);

/** The normal procedure for parsing an HTTP message is to read the
   start-line into a structure, read each header field into a hash table
   by field name until the empty line, and then use the parsed data to
   determine if a message body is expected.  If a message body has been
   indicated, then it is read as a stream until an amount of octets
   equal to the message body length is read or the connection is closed.  */
int http_parser_parse(struct http_parser *parser, FILE *fp);

int http_parser_feed_line(struct http_parser *parser, char *line);

/** feed a request line  */
int http_parser_feed_request_line(struct http_parser *parser, char *line);

/** feed header fields by line **/
int http_parser_feed_header_fields(struct http_parser *parser, char *line);

/** get error message from enum parser_state **/
const char *parse_error(enum parser_state state);

/** print parser information **/
void http_parser_print_information(struct http_parser *parser);

/** all other functions **/

int method(struct http_parser *p, char *s);

int uri(struct http_parser *p, char *s);

int protocol_version(struct http_parser *p, char *s);

int header_fields(struct http_parser *p, char *s);

int empty_line(struct http_parser *p, char *s);

int message_body(struct http_parser *p, char *s);

#endif
