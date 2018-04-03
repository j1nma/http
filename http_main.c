#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <errno.h>

#include "http_parser.h"

int main(int argc, char **argv)
{
    struct http_parser parser;
    http_parser_init(&parser);

    FILE *fp = argc > 1 ? fopen(argv[1], "r") : stdin;

    if (!fp)
    {
        fprintf(stderr, "Error: file open failed for '%s'.\n", argv[1]);
        return 1;
    }

    http_parser_parse(&parser, fp);

    http_parser_print_information(&parser);

    return 0;
}