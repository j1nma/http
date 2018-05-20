#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "include/http_parser.h"
#include "include/buffer.h"

// #define N(x) (sizeof(x) / sizeof(x[0]))

// #define FIXBUF(b, data)                 \
//     buffer_init(&(b), N(data), (data)); \
//     buffer_write_adv(&(b), N(data))

// int main_line(int argc, char **argv)
// {
//     struct http_parser *parser;
//     parser = http_parser_init();

//     FILE *fp = argc > 1 ? fopen(argv[1], "r") : stdin;

//     if (!fp)
//     {
//         fprintf(stderr, "Error: file open failed for '%s'.\n", argv[1]);
//         return -1;
//     }

//     int error = http_parser_parse(parser, fp);

//     if (error != 0)
//     {
//         return error;
//     }

//     http_parser_print_information(parser);

//     http_parser_free(parser);

//     return 0;
// }

/** OCTET **/

static int stdinFD = STDIN_FILENO;
static int stdoutFD = STDOUT_FILENO;

static void handleReadStdIn(struct buffer *buff)
{

    ssize_t bytesRead = 0;

    size_t write_bytes[1] = {0};

    uint8_t *ptr = buffer_write_ptr(buff, write_bytes);

    bytesRead = read(stdinFD, ptr, *write_bytes);

    if (bytesRead > 0)
    {

        buffer_write_adv(buff, bytesRead);

        // printf("===Read from stdin===\n%s", ptr);
    }
    else if (bytesRead == 0)
    {

        buffer_write_adv(buff, 0);

        close(stdinFD);
        stdinFD = -1;
    }
    else
    {

        close(stdinFD);
        stdinFD = -1;

        perror("StdIn read failed");
    }
}

static void handleWriteStdOut(struct buffer *buff)
{

    ssize_t bytesWritten = 0;

    size_t read_bytes[1] = {0};

    uint8_t *ptr = buffer_read_ptr(buff, read_bytes);

    bytesWritten = write(stdoutFD, ptr, *read_bytes);

    if (bytesWritten > 0)
    {

        buffer_read_adv(buff, bytesWritten);
    }
    else if (bytesWritten == 0)
    {

        buffer_read_adv(buff, 0);
    }
    else
    {

        perror("StdOut write failed");
    }
}

int main_octet(int argc, char **argv)
{
    struct http_parser *parser;
    parser = http_parser_init();

    uint8_t buffin[4096] = {0}, buffout[4096] = {0};
    buffer bin, bou;
    buffer_init(&bin, sizeof(buffin) / sizeof(*buffin), buffin);
    buffer_init(&bou, sizeof(buffout) / sizeof(*buffout), buffout);

    // if (stdinFD != -1 && FD_ISSET(stdinFD, &readSet))
    if (stdinFD != -1)
    {
        handleReadStdIn(&bin);
    }

    bool errored = false;

    enum parser_state pt = http_parser_consume(&bin, parser, &errored);

    if (errored)
    {
        return 1;
    }

    // if (stdoutFD != -1 && FD_ISSET(stdoutFD, &writeSet))
    if (stdoutFD != -1)
    {
        // handleWriteStdOut(&bou);
        http_parser_print_information(parser);
    }

    http_parser_free(parser);

    return 0;
}

/** MAIN **/

int main(int argc, char **argv)
{
    // main_line(argc, argv);
    main_octet(argc, argv);
}
