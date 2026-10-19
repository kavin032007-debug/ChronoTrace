#include "validation.h"
#include <ctype.h>
#include <string.h>

bool ct_validate_id_format(const char *id) {
    if (!id || id[0] == '\0') return false;
    size_t len = strlen(id);
    if (len >= CT_MAX_ID_LEN) return false;

    for (size_t i = 0; i < len; i++) {
        char c = id[i];
        if (!isalnum((unsigned char)c) && c != '_' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
}
