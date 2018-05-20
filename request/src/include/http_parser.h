#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>

#include "map.h"
#include "buffer.h"

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
    char *port;

    /** message body **/
    char *body;

    /** headers **/
    map_str_t header_map;
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
    parser_error_body,

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
    enum parser_state state;

    struct http_request *request;
};

/** parse functions **/
int method(struct http_parser *p, char *s);

int uri(struct http_parser *p, char *s);

int protocol_version(struct http_parser *p, char *s);

/** initialize parser **/
struct http_parser *http_parser_init(void);

/** free header field value from map **/
void free_header_fields_of_map(map_str_t map);

/** free parser **/
void http_parser_free(struct http_parser *parser);

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

/** feed message body by line **/
int http_parser_feed_body(struct http_parser *parser, char *line);

/** get error message from enum parser_state **/
const char *parse_error(enum parser_state state);

/** print parser information **/
void http_parser_print_information(struct http_parser *parser);

/** OCTET **/

/**
 * Permite distinguir a quien usa socks_hello_parser_feed si debe seguir
 * enviando caracters o no. 
 *
 * En caso de haber terminado permite tambien saber si se debe a un error
 */
bool http_parser_request_is_done(const enum parser_state st, bool *errored);

/**
 * por cada elemento del buffer llama a `request_parser_feed' hasta que
 * el parseo se encuentra completo o se requieren mas bytes.
 *
 * @param errored parametro de salida. si es diferente de NULL se deja dicho
 *   si el parsing se debió a una condición de error
 */
enum parser_state http_parser_consume(buffer *b, struct http_parser *parser, bool *errored);

/** feeds byte to parser  */
enum parser_state http_parser_feed(struct http_parser *parser, const uint8_t c);

/** feed byte to request line  */
enum parser_state http_parser_feed_byte_to_request_line(struct http_parser *parser, const uint8_t c);

/** feed header fields by byte **/
enum parser_state http_parser_feed_byte_to_header_fields(struct http_parser *parser, const uint8_t c);

/** feed message body by byte **/
enum parser_state http_parser_feed_byte_to_body(struct http_parser *parser, const uint8_t c);

#endif
