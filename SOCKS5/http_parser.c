#include <stdio.h>
#include <stdlib.h>

#include "http_parser.h"

extern void 
http_parser_init(struct http_parser *p) {
    p->state     = parser_version;
    p->remaining = 0;
}

extern enum parser_state
http_parser_feed(struct http_parser *p, const uint8_t b) {
    switch(p->state) {
        case parser_version:
            if(0x05 == b) {
                p->state = parser_nmethods;
            } else {
                p->state = parser_error_unsupported_version;
            }
            break;

        case parser_nmethods:
            p->remaining = b;
            p->state     = parser_methods;

            if(p->remaining <= 0) {
                p->state = parser_done;
            }

            break;

        case parser_methods:
            if(NULL != p->on_authentication_method) {
                p->on_authentication_method(p, b);
            }
            p->remaining--;
            if(p->remaining <= 0) {
                p->state = parser_done;
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

extern bool 
parser_is_done(const enum parser_state state, bool *errored) {
    bool ret;
    switch (state) {
        case parser_error_unsupported_version:
            if (0 != errored) {
                *errored = true;
            }
            /* no break */
        case parser_done:
            ret = true;
            break;
        default:
            ret = false;
            break;
    }
   return ret;
}

extern const char *
hello_error(const struct http_parser *p) {
    char *ret;
    switch (p->state) {
        case parser_error_unsupported_version:
            ret = "unsupported version";
            break;
        default:
            ret = "";
            break;
    }
    return ret;
}

extern void 
http_parser_close(struct http_parser *p) {
    /* no hay nada que liberar */
}

extern enum parser_state
parser_consume(buffer *b, struct http_parser *p, bool *errored) {
    enum parser_state st = p->state;

    while(buffer_can_read(b)) {
        const uint8_t c = buffer_read(b);
        st = http_parser_feed(p, c);
        if (parser_is_done(st, errored)) {
            break;
        }
    }
    return st;
}

extern int
http_marshall(buffer *b, const uint8_t method) {
    size_t n;
    uint8_t *buff = buffer_write_ptr(b, &n);
    if(n < 2) {
        return -1;
    }
    buff[0] = 0x05;
    buff[1] = method;
    buffer_write_adv(b, 2);
    return 2;
}

