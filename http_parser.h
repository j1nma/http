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
    char *method_token;
    char *uri;
    char *protocol_version;

    /** headers **/
    header_map_t header_map;

    /** message body **/
    char *body;
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

// /**
//  * por cada elemento del buffer llama a `http_parser_feed' hasta que
//  * el parseo se encuentra completo o se requieren mas bytes.
//  *
//  * @param errored parametro de salida. si es diferente de NULL se deja dicho
//  *   si el parsing se debió a una condición de error
//  */
// enum parser_state
// parser_consume(buffer *b, struct http_parser *p, bool *errored);

// /**
//  * Permite distinguir a quien usa http_parser_feed si debe seguir
//  * enviando caracters o no. 
//  *
//  * En caso de haber terminado permite tambien saber si se debe a un error
//  */
// bool parser_is_done(const enum parser_state state, bool *errored);

// /**
//  * En caso de que se haya llegado a un estado de error, permite obtener una
//  * representación textual que describe el problema
//  */
// extern const char *
// parser_error(const struct http_parser *p);

// /** libera recursos internos del parser */
// void http_parser_close(struct http_parser *p);

// /**
//  * serializa en buff la una respuesta al hello.
//  *
//  * Retorna la cantidad de bytes ocupados del buffer o -1 si no había
//  * espacio suficiente.
//  */
// int http_marshall(buffer *b, const uint8_t method);

#endif
