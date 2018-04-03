#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_parser.h"

#define BUFFER_SIZE 256
#define CRLF "\r\n"

char *request_methods[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE"};
char *protocol_versions[] = {"HTTP/1.0", "HTTP/1.1"};

static int calcParity(uint8_t *ptr, ssize_t size)
{
    unsigned char b = 0;
    int i;

    for (i = 0; i < size; i++)
    {
        b ^= ptr[i];
    }

    return b;
}

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
    parser.state = parser_request_line;
    parser.request = malloc(sizeof(parser.request));
    parser.request->header_map = hashmap_new();

    *p = parser;
}

int http_parser_parse(struct http_parser *parser, FILE *fp)
{

    char buf[BUFFER_SIZE] = "";
    char *delimeter = CRLF;
    char *line = buf;
    int error = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        line = strtok(buf, delimeter);

        while (line)
        {
            error = http_parser_feed_line(parser, line);
            if (error == 1)
            {
                fprintf(stderr, "Error: parsing line\n");
                return 1;
            }
            line = strtok(NULL, delimeter);
        }
    }

    if (ferror(fp))
    {
        puts("I/O error when reading.");
    }
    else if (feof(fp))
    {
        puts("End of file reached successfully.");
    }
    if (fp != stdin)
    {
        fclose(fp); /* close file if not stdin */
    }

    return 0;
}

int http_parser_feed_line(struct http_parser *parser, const char *line)
{
    switch (parser->state)
    {
    case parser_request_line:
        parser->state = parser_method;
        int error = http_parser_feed_request_line(parser, line);
        if (error == 1)
        {
            fprintf(stderr, "Error: %s\n", parse_error(parser->state));
            return error;
        }
        break;

    case parser_header_fields:

        break;
    case parser_empty_line:

        break;
    case parser_message_body:

        break;
    case parser_done:
    case parser_error_unsupported_version:
        // nada que hacer, nos quedamos en este estado
        break;
    default:
        fprintf(stderr, "unknown state %d\n", parser->state);
        abort();
    }

    return parser->state;
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

int http_parser_feed_request_line(struct http_parser *parser, char *line)
{

    char buf[BUFFER_SIZE] = "";
    char *delimeter = " ";
    char *token = buf;

    token = strtok(line, delimeter);

    if (token == NULL)
    {
        fprintf(stderr, "Error: could not parse request line.\n");
        parser->state = parser_error_request_line;
        return 1;
    }

    while (token != NULL)
    {

        switch (parser->state)
        {
        case parser_method:
            if (method(parser, token))
            {
                parser->state = parser_uri;
            }
            else
            {
                parser->state = parser_error_unsupported_method;
                fprintf(stderr, "Error: %s\n", parse_error(parser->state));
                return 1;
            }
            break;

        case parser_uri:
            if (uri(parser, token))
            {
                parser->state = parser_protocol_version;
            }
            else
            {
                parser->state = parser_error_unsupported_uri;
                fprintf(stderr, "Error: %s\n", parse_error(parser->state));
                return 1;
            }

            break;
        case parser_protocol_version:
            if (protocol_version(parser, token))
            {
                parser->state = parser_done;
            }
            else
            {
                parser->state = parser_error_unsupported_protocol_version;
                fprintf(stderr, "Error: %s\n", parse_error(parser->state));
                return 1;
            }
            break;
        case parser_done:
        case parser_error_unsupported_version:
            break;
        default:
            fprintf(stderr, "Unknown state: %d\n", parser->state);
            abort();
        }

        token = strtok(NULL, delimeter);
    }

    if (parser->state == parser_done)
    {
        return 0;
    }
    else
    {
        parser->state = parser_error_request_line;
        return 1;
    }
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

void http_parser_print_information(struct http_parser *parser)
{

    struct http_request *request = parser->request;
    header_map_t header_fields_map = request->header_map;

    int error;

    /** the method (section 3.1.1 Request Line of RFC7230) **/
    printf("%s", request->method_token);
    printf("\t");

    /** the host that must be used to make the connection (uri-host according to section 2.7. Resource Identifiers of RFC7230) **/
    char *host;

    error = hashmap_get(header_fields_map, "Host", (void **)(&host));
    assert(error == MAP_OK);

    printf("%s", host);
    printf("\t");

    /** the port where the connection will be made (decimal) **/
    strtok(host, ":");
    char *port = strtok(NULL, ":");
    if (port == NULL)
    {
        fprintf(stderr, "Port not found on host.\n");
    }
    else
    {
        printf("%s", port);
        printf("\t");
    }

    /** the request-target in form origin-form (section 5.3 Request Target of RFC7230) **/
    char *request_uri = request->uri;
    char *request_target;

    strtok(request_uri, "http://");
    request_target = strtok(NULL, "/");
    if (request_target == NULL)
    {
        fprintf(stderr, "Request-target not found on host.\n");
    }
    else
    {
        printf("/%s", request_target);
        printf("\t");
    }

    printf("%s", request_target);
    printf("\t");

    /** the number of bytes of message body (decimal) **/
    char *message_body_bytes;
    error = hashmap_get(header_fields_map, "Content-Length", (void **)(&message_body_bytes));
    if (error == MAP_MISSING)
    {
        message_body_bytes = "0";
    }

    printf("%s", message_body_bytes);
    printf("\t");

    /** one byte (hexadecimal format always showing the two octets in upper case) that is the product of applying XOR between all the bytes of the body of the order (initialized in 0). **/
    printf("%X", calcParity(request->body, sizeof(request->body)));
    printf("\t");
}