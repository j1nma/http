#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_parser.h"

#define BUFFER_SIZE 256
#define CRLF "\r\n"

char *request_methods[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE"};
char *protocol_versions[] = {"HTTP/1.0", "HTTP/1.1"};

void printBits(size_t const size, void const *const ptr)
{
    unsigned char *b = (unsigned char *)ptr;
    unsigned char byte;
    int i, j;

    for (i = size - 1; i >= 0; i--)
    {
        for (j = 7; j >= 0; j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

static void removeSubstring(char *s, const char *toRemove)
{
    unsigned long toRemoveLen = strlen(toRemove);
    while ((s = strstr(s, toRemove)))
        memmove(s, s + toRemoveLen, 1 + strlen(s + toRemoveLen));
}

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
            strcpy(p->request->method_token, request_methods[i]);
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

    strcpy(p->request->uri, s);

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
            strcpy(p->request->protocol_version, protocol_versions[i]);
        }
    }

    if (sc != 0)
    {
        return 0;
    }

    return 1;
}

struct http_parser *http_parser_init()
{
    struct http_parser *parser = malloc(sizeof(struct http_parser));
    if (parser == NULL)
    {
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    parser->request = malloc(sizeof(struct http_request));
    if (parser->request == NULL)
    {
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    parser->request->method_token = malloc(BUFFER_SIZE * sizeof(char));
    if (parser->request->method_token == NULL)
    {
        free(parser->request);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->request->method_token, 0, sizeof(char) * BUFFER_SIZE);

    parser->request->uri = malloc(BUFFER_SIZE * sizeof(char));
    if (parser->request->uri == NULL)
    {
        free(parser->request->method_token);
        free(parser->request);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->request->uri, 0, sizeof(char) * BUFFER_SIZE);

    parser->request->protocol_version = malloc(BUFFER_SIZE * sizeof(char));
    if (parser->request->protocol_version == NULL)
    {
        free(parser->request->uri);
        free(parser->request->method_token);
        free(parser->request);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->request->protocol_version, 0, sizeof(char) * BUFFER_SIZE);

    parser->request->port = malloc(BUFFER_SIZE * sizeof(char));
    if (parser->request->port == NULL)
    {
        free(parser->request->protocol_version);
        free(parser->request->uri);
        free(parser->request->method_token);
        free(parser->request);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->request->port, 0, sizeof(char) * BUFFER_SIZE);

    parser->request->body = malloc(BUFFER_SIZE * BUFFER_SIZE * sizeof(char));
    if (parser->request->body == NULL)
    {
        free(parser->request->port);
        free(parser->request->protocol_version);
        free(parser->request->uri);
        free(parser->request->method_token);
        free(parser->request);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->request->body, 0, sizeof(char) * BUFFER_SIZE * BUFFER_SIZE);

    map_init(&parser->request->header_map);

    parser->state = parser_request_line;

    return parser;
}

void http_parser_free(struct http_parser *parser)
{

    if (parser != NULL)
    {
        if (parser->request != NULL)
        {
            free(parser->request->method_token);
            free(parser->request->uri);
            free(parser->request->protocol_version);
            free(parser->request->port);
            free(parser->request->body);
            map_deinit_((map_base_t *)&(parser->request->header_map));
            free(parser->request);
        }

        free(parser);
    }
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
        line = strtok(buf, delimeter);

        error = http_parser_feed_line(parser, line);
        if (error == -1)
        {
            fprintf(stderr, "Error: %s\n", parse_error(parser->state));
            return error;
        }
    }

    if (ferror(fp))
    {
        fprintf(stderr, "I/O error when reading.");
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
            // parser->request->header_map = hashmap_new();
        }

        break;

    case parser_header_fields:

        /** if empty line with LF only is reached, header fields are done **/
        if (line == NULL)
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

        parser->state = parser_message_body;

        break;
    case parser_message_body:

        error = http_parser_feed_body(parser, line);
        if (error == -1)
        {
            fprintf(stderr, "Error: %s\n", parse_error(parser->state));
            return error;
        }

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
    char *delimeter = ": ";

    field_name = malloc(BUFFER_SIZE);
    if (field_name == NULL)
    {
        perror("Error allocating memory");
        abort();
    }

    field_value = malloc(BUFFER_SIZE);
    if (field_value == NULL)
    {
        perror("Error allocating memory");
        abort();
    }

    memset(field_name, 0, BUFFER_SIZE);
    memset(field_value, 0, BUFFER_SIZE);

    strtok(line, CRLF);

    strcpy(field_name, strtok(line, delimeter));

    if (field_name == NULL)
    {
        fprintf(stderr, "Error: could not parse header field name.\n");
        parser->state = parser_error_header_field;
        return -1;
    }

    strcpy(field_value, strtok(NULL, delimeter));

    if (field_value == NULL)
    {
        fprintf(stderr, "Error: could not parse header field value.\n");
        parser->state = parser_error_header_field;
        return -1;
    }

    if (!strcmp(field_name, "Host"))
    {
        char *host_port = strtok(NULL, delimeter);

        if (host_port == NULL)
        {
            strcpy(parser->request->port, "80");
        }
        else
        {
            strcpy(parser->request->port, host_port);
        }
    }

    error = map_set(&parser->request->header_map, field_name, field_value);

    if (error != 0)
    {
        fprintf(stderr, "Error: parsing header field.\n");
        parser->state = parser_error_header_field;
        return -1;
    }

    // free(field_value);
    free(field_name);

    return 0;
}

int http_parser_feed_body(struct http_parser *parser, char *line)
{

    int lineLen = strlen(line);
    int bodySize = strlen(parser->request->body);
    int bodyIndex = bodySize;

    for (int i = 0; i < lineLen; i++, bodyIndex++)
    {

        /* Check if we need to expand. */
        if (bodySize <= i)
        {
            bodySize = bodySize + 1;

            char *tmp = NULL;
            tmp = realloc(parser->request->body, bodySize);

            if (!tmp)
            {
                parser->state = parser_error_body;
                return -1;
            }

            parser->request->body = tmp;
        }

        /* Actually store the thing. */
        parser->request->body[bodyIndex] = line[i];
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
    case parser_error_body:
        error_message = "error on message body";
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

    /** the method (section 3.1.1 Request Line of RFC7230) **/
    printf("%s", parser->request->method_token);
    printf("\t");

    /** the host that must be used to make the connection (uri-host according to section 2.7. Resource Identifiers of RFC7230) **/
    char **host = map_get(&parser->request->header_map, "Host");
    if (host)
    {
        printf("%s\t", *host);
    }
    else
    {
        fprintf(stderr, "Host not found.\n");
        return;
    }

    /** the port where the connection will be made (decimal) **/

    printf("%s\t", parser->request->port);

    /** the request-target in form origin-form (section 5.3 Request Target of RFC7230) **/
    char *request_uri = parser->request->uri;
    char *request_target;

    char *aux_request_target = malloc(BUFFER_SIZE);
    if (aux_request_target == NULL)
    {
        perror("Error allocating memory");
        abort();
    }
    memset(aux_request_target, 0, BUFFER_SIZE);

    int prevLen = strlen(request_uri);
    removeSubstring(request_uri, "http://");
    int postLen = strlen(request_uri);
    int hadHTTP = prevLen - postLen;

    if (hadHTTP != 0)
    {
        strtok(request_uri, "/");

        request_target = strtok(NULL, "/");

        if (request_target == NULL)
        {
            fprintf(stderr, "Request-target not found on uri.\n");
            return;
        }

        printf("/%s\t", request_target);
    }
    else
    {
        printf("%s\t", request_uri);
    }

    /** the number of bytes of message body (decimal) **/
    char **message_body_bytes = map_get(&parser->request->header_map, "Content-Length");
    if (message_body_bytes)
    {
        printf("%s\t", *message_body_bytes);
    }
    else
    {
        printf("%s\t", "0");
    }

    /** one byte (hexadecimal format always showing the two octets in upper case) that is the product of applying XOR between all the bytes of the body of the order (initialized in 0). **/
    if (message_body_bytes)
    {
        int parity = calcParity((uint8_t *)parser->request->body, strlen(parser->request->body) + 1);
        printf("%X\n", parity);
    }
    else
    {
        printf("%02X", 0);
    }
}
