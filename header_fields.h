#define KEY_MAX_LENGTH (256)

typedef struct header_field_s
{
    char key_string[KEY_MAX_LENGTH];
    char value_string[KEY_MAX_LENGTH];

} header_field_t;