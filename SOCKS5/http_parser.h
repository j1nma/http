#include <stdint.h>
#include <stdbool.h>

#include "buffer.h"

static const uint8_t METHOD_NO_AUTHENTICATION_REQUIRED = 0x00;
static const uint8_t METHOD_NO_ACCEPTABLE_METHODS      = 0xFF;

/*
 *   The client connects to the server, and sends a version
 * identifier/method selection message:
 *
 *                 +----+----------+----------+
 *                 |VER | NMETHODS | METHODS  |
 *                 +----+----------+----------+
 *                 | 1  |    1     | 1 to 255 |
 *                 +----+----------+----------+
 *
 *  The VER field is set to X'05' for this version of the protocol.  The
 *  NMETHODS field contains the number of method identifier octets that
 *  appear in the METHODS field.
 */
/* no es un ADT/CDT para no evitar malloc/free */

enum parser_state {
    parser_version,
    /** reading number of methods */
    parser_nmethods,
    /** reading methods */
    parser_methods,
    parser_done,
    parser_error_unsupported_version,
};

struct http_parser {
    /** called for each new method */
    void (*on_authentication_method) 
         (struct http_parser *parser, const uint8_t method);

    /** parser user can store data */
    void *data;

    /******** private *****************/
    enum parser_state state;
    
    /* methods remaining to be read */
    uint8_t remaining;
};

void http_parser_init (struct http_parser *p);

/** feed a byte to parser. return true of reached the end  */
enum parser_state http_parser_feed (struct http_parser *p, uint8_t b);

/**
 * por cada elemento del buffer llama a `http_parser_feed' hasta que
 * el parseo se encuentra completo o se requieren mas bytes.
 *
 * @param errored parametro de salida. si es diferente de NULL se deja dicho
 *   si el parsing se debió a una condición de error
 */
enum parser_state
parser_consume(buffer *b, struct http_parser *p, bool *errored);

/**
 * Permite distinguir a quien usa http_parser_feed si debe seguir
 * enviando caracters o no. 
 *
 * En caso de haber terminado permite tambien saber si se debe a un error
 */
bool 
parser_is_done(const enum parser_state state, bool *errored);

/**
 * En caso de que se haya llegado a un estado de error, permite obtener una
 * representación textual que describe el problema
 */
extern const char *
parser_error(const struct http_parser *p);


/** libera recursos internos del parser */
void http_parser_close(struct http_parser *p);

static const uint8_t SOCKS_HELLO_NOAUTHENTICATION_REQUIRED = 0x00;
/*
 * If the selected METHOD is X'FF', none of the methods listed by the
   client are acceptable, and the client MUST close the connection.
 */
static const uint8_t SOCKS_HELLO_NO_ACCEPTABLE_METHODS = 0xFF;


/**
 * serializa en buff la una respuesta al hello.
 *
 * Retorna la cantidad de bytes ocupados del buffer o -1 si no había
 * espacio suficiente.
 */
int
http_marshall(buffer *b, const uint8_t method);
