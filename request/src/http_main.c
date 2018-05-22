#include <stdio.h>
#include <stdlib.h>

#include "include/http_parser.h"

#define BIN_BUFFER_SIZE 4096

/**
 * Reads bytes from *fd onto *buff.
 */
static int doread(int *fd, buffer *buff)
{
    uint8_t *ptr;
    ssize_t n;
    size_t count = 0;
    int ret = 0;

    ptr = buffer_write_ptr(buff, &count);
    n = read(*fd, ptr, count);
    if (n == 0 || n == -1)
    {
        /* Close file descriptor */
        *fd = -1;
        ret = -1;
    }
    else
    {
        buffer_write_adv(buff, n);
    }

    return ret;
}

int main(int argc, char **argv)
{
    struct http_parser *parser;
    parser = http_parser_init();

    uint8_t buffin[BIN_BUFFER_SIZE] = {0};
    buffer bin;
    buffer_init(&bin, sizeof(buffin) / sizeof(*buffin), buffin);

    int fd = STDIN_FILENO;

    int error = doread(&fd, &bin);

    if (error != 0)
    {
        printf("Could not read from %d into buffer.\n", fd);
        return error;
    }

    error = http_parser_parse(parser, &bin);

    if (error != 0)
    {
        return error;
    }

    // http_parser_print_information(parser);

    // http_parser_free(parser);

    return 0;
}
