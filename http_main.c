#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <errno.h>

#include "http_parser.h"

#define BUFFER_SIZE 256

int main(int argc, char **argv)
{
    char buf[BUFFER_SIZE] = "",
         *delimeter = " ",
         *p = buf;

    struct http_parser parser;
    http_parser_init(&parser);

    parser.request = malloc(sizeof(struct http_request));
    parser.request->header_map = hashmap_new();

    FILE *fp = argc > 1 ? fopen(argv[1], "r") : stdin;

    if (!fp)
    {
        fprintf(stderr, "error: file open failed '%s'.\n", argv[1]);
        return 1;
    }

    while (fgets(buf, sizeof buf, fp) != NULL)
    {
        p = strtok(buf, delimeter);

        while (p)
        {
            printf("%s\n", p);
            http_parser_feed(&parser, p);
            p = strtok(NULL, delimeter); /* get remaining tokens */
        }
    }

    if (ferror(fp))
        puts("I/O error when reading");
    else if (feof(fp))
        puts("End of file reached successfully");

    if (fp != stdin)
        fclose(fp); /* close file if not stdin */

    return 0;
}
