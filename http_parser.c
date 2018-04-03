#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_parser.h"

char *request_methods[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE"};
char *protocol_versions[] = {"HTTP/1.0", "HTTP/1.1"};

#define str(s) #s

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

    return 1;
}

int protocol_version(struct http_parser *p, const char *s)
{

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
    p = malloc(sizeof(*p));
    p->state = parser_method;
    p->remaining = 0;
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
            printf("found method: %s\n", p->request->method_token);
        }
        else
        {
            p->state = parser_error_unsupported_method;
            fprintf(stderr, "error: %d\n", p->state);
        }
        break;

    case parser_uri:
        if (uri(p, s))
        {
            p->state = parser_protocol_version;
        }
        else
        {
            p->state = parser_error_unsupported_uri;
            fprintf(stderr, "error: %d\n", p->state);
        }

        break;
    case parser_protocol_version:
        if (protocol_version(p, s))
        {
            p->state = parser_header_fields;
        }
        else
        {
            p->state = parser_error_unsupported_protocol_version;
            fprintf(stderr, "error: %d\n", p->state);
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
            fprintf(stderr, "error: %d\n", p->state);
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
            fprintf(stderr, "error: %d\n", p->state);
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
            fprintf(stderr, "error: %d\n", p->state);
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

// extern bool
// parser_is_done(const enum parser_state state, bool *errored)
// {
//     bool ret;
//     switch (state)
//     {
//     case parser_error_unsupported_version:
//         if (0 != errored)
//         {
//             *errored = true;
//         }
//         /* no break */
//     case parser_done:
//         ret = true;
//         break;
//     default:
//         ret = false;
//         break;
//     }
//     return ret;
// }

// extern const char *
// hello_error(const struct http_parser *p)
// {
//     char *ret;
//     switch (p->state)
//     {
//     case parser_error_unsupported_version:
//         ret = "unsupported version";
//         break;
//     default:
//         ret = "";
//         break;
//     }
//     return ret;
// }

// extern void
// http_parser_close(struct http_parser *p)
// {
//     /* no hay nada que liberar */
// }

// extern enum parser_state
// parser_consume(buffer *b, struct http_parser *p, bool *errored)
// {
//     enum parser_state st = p->state;

//     while (buffer_can_read(b))
//     {
//         const uint8_t c = buffer_read(b);
//         st = http_parser_feed(p, c);
//         if (parser_is_done(st, errored))
//         {
//             break;
//         }
//     }
//     return st;
// }

// extern int
// http_marshall(buffer *b, const uint8_t method)
// {
//     size_t n;
//     uint8_t *buff = buffer_write_ptr(b, &n);
//     if (n < 2)
//     {
//         return -1;
//     }
//     buff[0] = 0x05;
//     buff[1] = method;
//     buffer_write_adv(b, 2);
//     return 2;
// }
