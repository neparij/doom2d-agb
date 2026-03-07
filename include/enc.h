#ifndef DOOM2D_ENC_H
#define DOOM2D_ENC_H

#include <stddef.h>
#include "bn_string.h"

#define CP866_STRING_MAX_LEN 20

bn::string<CP866_STRING_MAX_LEN> cp866_to_utf8(const char* cp866_str);
bn::string<CP866_STRING_MAX_LEN> cp866_to_utf8(const unsigned char* cp866_str);

/** Конвертирует UTF-8 строку (кириллица + ASCII) в CP866. Пишет в out до out_max байт, без нуль-терминатора. */
int utf_to_cp866(const char* utf8, unsigned char* out, int out_max);

/** Приводит 8 байт CP866 к верхнему регистру (ASCII a-z, кириллица а-я/ё -> А-Я/Е). Пишет в out. */
void cp866_upper_8(const unsigned char* in, unsigned char* out);

#endif //DOOM2D_ENC_H