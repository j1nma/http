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

#include <regex.h>
static void regexTest()
{
    regex_t regex;
    int reti;
    char msgbuf[100];

    /* Compile regular expression */
    reti = regcomp(&regex, "[^ \f\n\r\t\v]", 0);
    if (reti)
    {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    /* Execute regular expression */
    reti = regexec(&regex, "ab c", 0, NULL, 0);
    if (!reti)
    {
        puts("Match");
    }
    else if (reti == REG_NOMATCH)
    {
        puts("No match");
    }
    else
    {
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        exit(1);
    }

    /* Free memory allocated to the pattern buffer by regcomp() */
    regfree(&regex);
}

int main(int argc, char **argv)
{

    // regexTest();
    struct http_parser *parser;
    parser = http_parser_init();

    /* bin is a buffer where the client request should be buffered */
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

    http_parser_print_information(parser);

    // http_parser_free(parser);

    return 0;
}
