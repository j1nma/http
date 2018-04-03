#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_parser.h"

char *request_methods[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE"};
char *protocol_versions[] = {"HTTP/1.0", "HTTP/1.1"};

int method(struct http_parser *p, const char *s)
{
    int len = sizeof(request_methods) / sizeof(request_methods[0]);
    int i;
    int sc = 1;

    for (i = 0; i < len && sc; ++i)
    {

        sc = strcmp(request_methods[i], s);

        if (!sc)
        {
            p->request->method_token = request_methods[i];
        }
    }

    if (sc != 0)
    {
        return 0;
    }

    return 1;
}

int uri(struct http_parser *p, const char *s)
{

    p->request->uri = s;

    return 1;
}

int protocol_version(struct http_parser *p, const char *s)
{
    int len = sizeof(protocol_versions) / sizeof(protocol_versions[0]);
    int i;
    int sc = 1;

    for (i = 0; i < len && sc; ++i)
    {

        sc = strcmp(protocol_versions[i], s);

        if (!sc)
        {
            p->request->protocol_version = protocol_versions[i];
        }
    }

    if (sc != 0)
    {
        return 0;
    }

    return 1;
}

int header_fields(struct http_parser *p, const char *s)
{

    return 1;
}

int empty_line(struct http_parser *p, const char *s)
{

    return 1;
}

int message_body(struct http_parser *p, const char *s)
{

    return 1;
}

extern void
http_parser_init(struct http_parser *p)
{
    struct http_parser parser;
    parser.state = parser_method;
    parser.remaining = 0;
    parser.request = malloc(sizeof(parser.request));
    parser.request->header_map = hashmap_new();

    *p = parser;
}

extern enum parser_state
http_parser_feed(struct http_parser *p, const char *s)
{
    switch (p->state)
    {
    case parser_method:
        if (method(p, s))
        {
            p->state = parser_uri;
            printf("Found method: %s\n", p->request->method_token);
        }
        else
        {
            p->state = parser_error_unsupported_method;
            fprintf(stderr, "Error: %s\n", parse_error(p->state));
        }
        break;

    case parser_uri:
        if (uri(p, s))
        {
            p->state = parser_protocol_version;
            printf("Found uri: %s\n", p->request->uri);
        }
        else
        {
            p->state = parser_error_unsupported_uri;
            fprintf(stderr, "Error: %s\n", parse_error(p->state));
        }

        break;
    case parser_protocol_version:
        if (protocol_version(p, s))
        {
            p->state = parser_done;
            printf("Found protocol version: %s\n", p->request->protocol_version);
        }
        else
        {
            p->state = parser_error_unsupported_protocol_version;
            fprintf(stderr, "Error: %s\n", parse_error(p->state));
        }
        break;
    case parser_header_fields:
        if (header_fields(p, s))
        {
            p->state = parser_empty_line;
        }
        else
        {
            p->state = parser_error_unsupported_header_fields;
            fprintf(stderr, "Error: %s\n", parse_error(p->state));
        }
        break;
    case parser_empty_line:
        if (empty_line(p, s))
        {
            p->state = parser_message_body;
        }
        else
        {
            p->state = parser_error_unsupported_empty_line;
            fprintf(stderr, "Error: %s\n", parse_error(p->state));
        }
        break;
    case parser_message_body:
        if (message_body(p, s))
        {
            p->state = parser_message_body;
        }
        else
        {
            p->state = parser_error_unsupported_message_body;
            fprintf(stderr, "Error: %s\n", parse_error(p->state));
        }
        break;
    case parser_done:
    case parser_error_unsupported_version:
        // nada que hacer, nos quedamos en este estado
        break;
    default:
        fprintf(stderr, "unknown state %d\n", p->state);
        abort();
    }

    return p->state;
}

enum parser_state http_parser_feed_request_line(struct http_parser *parser, char *line)
{

    char buf[256] = "",
         *delimeter = " ",
         *p = buf;

    p = strtok(line, delimeter);

    if (p == NULL)
    {
        fprintf(stderr, "Error: could not parse request line.\n");
        parser->state = parser_error_unsupported_method;
        return parser->state;
    }

    while (p)
    {
        // printf("%s\n", p);
        http_parser_feed(parser, p);
        p = strtok(NULL, delimeter); /* get remaining tokens */
    }

    return parser->state;
}

const char *parse_error(enum parser_state state)
{
    char *error_message;

    switch (state)
    {
    case parser_error_unsupported_method:
        error_message = "unsupported method";
        break;
    case parser_error_unsupported_uri:
        error_message = "unsupported uri";
        break;

    case parser_error_unsupported_protocol_version:
        error_message = "unsupported protocol version";
        break;

    case parser_error_unsupported_header_fields:
        error_message = "unsupported header fields";
        break;

    case parser_error_unsupported_empty_line:
        error_message = "unsupported empty line";
        break;

    case parser_error_unsupported_message_body:
        error_message = "unsupported message body";
        break;

    case parser_error_unsupported_version:
        error_message = "unsupported version";
        break;

    default:
        error_message = "unsupported error message for state";
        break;
    }

    return error_message;
}