#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/http_parser.h"

#define BUFFER_SIZE         256
#define CRLF                "\r\n"
#define PROTOCOL_LENGTH     8
#define MESSAGE_STATUS_SIZE 3

char *protocol_versions[] = {"HTTP/1.0", "HTTP/1.1"};

static char *concat(const char *s1, const char *s2)
{
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1);

    if (result == NULL)
    {
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
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

int status_code(struct http_parser *p, char *stat)
{
    strcpy(p->response->status, stat);
    return 1;
}

int protocol_version(struct http_parser *p, char *s) //ready
{
    int len = sizeof(protocol_versions) / sizeof(protocol_versions[0]);
    int i;
    int sc = 1;

    for (i = 0; i < len && sc; ++i)
    {

        sc = strcmp(protocol_versions[i], s);

        if (sc == 0)
        {
            strcpy(p->response->protocol_version, protocol_versions[i]);
        }
    }

    if (sc != 0)
    {
        return 0;
    }

    return 1;
}

struct http_parser *http_parser_init() //ready
{
    struct http_parser *parser = malloc(sizeof(struct http_parser));
    if (parser == NULL)
    {
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    parser->response = malloc(sizeof(struct http_response));
    if (parser->response == NULL)
    {
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }

    parser->response->status = malloc(MESSAGE_STATUS_SIZE * sizeof(int));
    if (parser->response->status == NULL)
    {
        free(parser->response);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->response->status, 0, sizeof(char) * MESSAGE_STATUS_SIZE);

    parser->response->protocol_version = malloc(BUFFER_SIZE * sizeof(char));
    if (parser->response->protocol_version == NULL)
    {
        free(parser->response->status);
        free(parser->response);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->response->protocol_version, 0, sizeof(char) * BUFFER_SIZE);

    parser->response->body = malloc(BUFFER_SIZE * BUFFER_SIZE * sizeof(char));
    if (parser->response->body == NULL)
    {
        free(parser->response->protocol_version);
        free(parser->response->status);
        free(parser->response);
        free(parser);
        fprintf(stderr, "Not enough memory\n");
        return NULL;
    }
    memset(parser->response->body, 0, sizeof(char) * BUFFER_SIZE * BUFFER_SIZE);

    map_init(&parser->response->header_map);

    parser->state = parser_response_line;

    return parser;
}

void free_header_fields_of_map(map_str_t map) //ready
{
    const char *key;
    map_iter_t iter = map_iter(&map);

    while ((key = map_next(&map, &iter)))
    {

        free(*map_get(&map, key));
    }
}

void http_parser_free(struct http_parser *parser)//ready
{

    if (parser != NULL)
    {
        if (parser->response != NULL)
        {
            free(parser->response->status);
            free(parser->response->body);
            free(parser->response->protocol_version);
            free_header_fields_of_map(parser->response->header_map);
            map_deinit_((map_base_t *)&(parser->response->header_map));
            free(parser->response);
        }

        free(parser);
    }
}

int http_parser_parse(struct http_parser *parser, FILE *fp)//ready
{

    char buf[BUFFER_SIZE] = "";
    char *line = buf;
    int error;

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {

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
    case parser_response_line:

        strtok(line, CRLF);

        parser->state = parser_protocol_version;

        error = http_parser_feed_response_line(parser, line);
        if (error == -1)
        {
            fprintf(stderr, "Error: %s\n", parse_error(parser->state));
            return error;
        }
        else
        {
            parser->state = parser_header_fields;
        }

        break;

    case parser_header_fields:

        line = strtok(line, CRLF);

        /** if empty line with LF only is reached, header fields are done **/
        if (!line)
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
        /** never reached because EOF ends line parsing **/
        break;
    default:
        fprintf(stderr, "unknown state %d\n", parser->state);
        abort();
    }

    return 0;
}

int http_parser_feed_response_line(struct http_parser *parser, char *line) //ready
{

    char buf[BUFFER_SIZE] = "";
    char *delimeter = " ";
    char *token = buf;

    token = strtok(line, delimeter);

    if (token == NULL)
    {
        fprintf(stderr, "Error: could not parse request line.\n");
        parser->state = parser_error_response_line;
        return -1;
    }

    while (token != NULL)
    {

        switch (parser->state)
        {
        case parser_protocol_version:
            if (protocol_version(parser,line))
            {
                parser->state = parser_status_code;
            }
            else
            {
                parser->state = parser_error_unsupported_protocol_version;
                return -1;
            }
            break;

        case parser_status_code:
            if (status_code(parser, token))
            {
                parser->state = parser_done;
            }
            else
            {
                parser->state = parser_error_unsupported_state;
                return -1;
            }
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

    error = map_set(&parser->response->header_map, field_name, field_value);

    if (error != 0)
    {
        fprintf(stderr, "Error: parsing header field.\n");
        parser->state = parser_error_header_field;
        return -1;
    }

    free(field_name);

    return 0;
}

int http_parser_feed_body(struct http_parser *parser, char *line)
{

    parser->response->body = concat(parser->response->body, line);

    if (parser->response->body == NULL)
    {
        fprintf(stderr, "Error: could not parse message body.\n");
        parser->state = parser_error_body;
        return -1;
    }

    return 0;
}

const char *parse_error(enum parser_state state)
{
    char *error_message;

    switch (state)
    {
    case parser_error_response_line:
        error_message = "error on status line";
        break;
    case parser_error_header_field:
        error_message = "error on header field";
        break;
    case parser_error_body:
        error_message = "error on message body";
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
    default:
        error_message = "unsupported error message for state";
        break;
    }

    return error_message;
}

void http_parser_print_information(struct http_parser *parser)
{

    /** the method (section 3.1.1 Request Line of RFC7230) **/
    printf("%s\t", parser->response->protocol_version);
    printf("%s\t", parser->response->status);
    printf("%s\n", parser->response->body);

}