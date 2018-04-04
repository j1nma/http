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

int method(struct http_parser *p, char *s)
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

int uri(struct http_parser *p, char *s)
{

    p->request->uri = s;

    return 1;
}

int protocol_version(struct http_parser *p, char *s)
{
    int len = sizeof(protocol_versions) / sizeof(protocol_versions[0]);
    int i;
    int sc = 1;

    for (i = 0; i < len && sc; ++i)
    {

        sc = strcmp(protocol_versions[i], s);

        if (sc == 0)
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

int header_fields(struct http_parser *p, char *s)
{

    return 1;
}

int empty_line(struct http_parser *p, char *s)
{

    return 1;
}

int message_body(struct http_parser *p, char *s)
{

    return 1;
}

extern void
http_parser_init(struct http_parser *p)
{
    struct http_parser parser;
    parser.state = parser_request_line;
    parser.request = malloc(sizeof(parser.request));

    *p = parser;
}

int http_parser_parse(struct http_parser *parser, FILE *fp)
{

    char buf[BUFFER_SIZE] = "";
    char *delimeter = CRLF;
    char *line = buf;
    int error;

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        /** when there are no tokens left to retrieve, strtok returns NULL **/
        // line = strtok(buf, delimeter);

        printf("current line: %s\n", line);

        error = http_parser_feed_line(parser, line);
        if (error == -1)
        {
            fprintf(stderr, "Error: %s\n", parse_error(parser->state));
            return error;
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
        fclose(fp);
    }

    return 0;
}

int http_parser_feed_line(struct http_parser *parser, char *line)
{
    int error;

    switch (parser->state)
    {
    case parser_request_line:

        parser->state = parser_method;

        error = http_parser_feed_request_line(parser, line);
        if (error == -1)
        {
            fprintf(stderr, "Error: %s\n", parse_error(parser->state));
            return error;
        }
        else
        {
            parser->state = parser_header_fields;
            parser->request->header_map = hashmap_new();
        }

        break;

    case parser_header_fields:

        /** if empty line with LF only is reached, header fields are done **/
        if (line[0] == '\n')
        {
            parser->state = parser_empty_line;
            break;
        }

        error = http_parser_feed_header_fields(parser, line);
        if (error == -1)
        {
            fprintf(stderr, "Error: %s\n", parse_error(parser->state));
            return error;
        }

        break;
    case parser_empty_line:

        parser->state = parser_done;

        break;
    case parser_message_body:

        break;
    case parser_done:
        break;
    default:
        fprintf(stderr, "unknown state %d\n", parser->state);
        abort();
    }

    return 0;
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
        return -1;
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
                return -1;
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
                return -1;
            }

            break;
        case parser_protocol_version:

            token = strtok(token, CRLF);

            if (protocol_version(parser, token))
            {
                parser->state = parser_done;
            }
            else
            {
                parser->state = parser_error_unsupported_protocol_version;
                return -1;
            }
            break;
        case parser_done:
            break;
        default:
            fprintf(stderr, "Unknown state: %d\n", parser->state);
            abort();
        }

        token = strtok(NULL, delimeter);

        if ((parser->state == parser_done && token != NULL) || (parser->state != parser_done && token == NULL))
        {
            fprintf(stderr, "Error: request line with wrong format\n");
            return -1;
        }
    }

    return 0;
}

int http_parser_feed_header_fields(struct http_parser *parser, char *line)
{

    int error;
    char *field_name;
    char *field_value;

    field_name = malloc(sizeof(*field_name));
    field_value = malloc(sizeof(*field_value));

    char *delimeter = ": ";

    line = strtok(line, CRLF);

    field_name = strtok(line, delimeter);

    if (field_name == NULL)
    {
        fprintf(stderr, "Error: could not parse header field name.\n");
        parser->state = parser_error_header_field;
        return -1;
    }

    field_value = strtok(NULL, delimeter);

    if (field_value == NULL)
    {
        fprintf(stderr, "Error: could not parse header field value.\n");
        parser->state = parser_error_header_field;
        return -1;
    }

    error = hashmap_put(parser->request->header_map, field_name, field_value);

    if (error != MAP_OK)
    {
        fprintf(stderr, "Error: parsing header field.\n");
        parser->state = parser_error_header_field;
        return -1;
    }

    return 0;
}

const char *parse_error(enum parser_state state)
{
    char *error_message;

    switch (state)
    {
    case parser_error_request_line:
        error_message = "error on request line";
        break;
    case parser_error_header_field:
        error_message = "error on header field";
        break;
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
    map_t header_fields_map = request->header_map;

    int error;

    /** the method (section 3.1.1 Request Line of RFC7230) **/
    printf("%s", request->method_token);
    printf("\t");

    /** the host that must be used to make the connection (uri-host according to section 2.7. Resource Identifiers of RFC7230) **/
    char *host;

    printf("header fields map length: %d\n", hashmap_length(header_fields_map));

    if (hashmap_length(header_fields_map) > 0)
    {
        error = hashmap_get(header_fields_map, "Host", (void **)(&host));

        if (error != MAP_OK)
        {
            fprintf(stderr, "Host not found.\n");
            return;
        }
        else
        {
            printf("%s", host);
            printf("\t");
        }
    }
    else
    {
        fprintf(stderr, "Error: header fields couldn't be retrieved.\n");
        return;
    }

    /** the port where the connection will be made (decimal) **/
    strtok(host, ":");
    char *port = strtok(NULL, ":");
    if (port == NULL)
    {
        /** Port not found on host: default goes to 80 **/
        printf("%s", "80");
        printf("\t");
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
        fprintf(stderr, "Request-target not found on uri.\n");
        return;
    }
    else
    {
        printf("/%s", request_target);
        printf("\t");
    }

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
    if (error == MAP_OK)
    {
        printf("%X", calcParity((uint8_t *)request->body, sizeof(request->body)));
        printf("\t");
    }
    else
    {
        printf("%X", 0);
    }
}
