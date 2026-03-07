#include "enc.h"
#include "bn_string.h"

inline bn::string<CP866_STRING_MAX_LEN> _cp866_to_utf8_impl(const unsigned char* cp866_str) {
    bn::string<CP866_STRING_MAX_LEN> utf8_str;
    while (*cp866_str) {
        uint8_t c = (uint8_t)*cp866_str++;
        if (c < 128) {
            utf8_str += (char)c;
        } else {
            // CP866: 0x80-0x9F = А-Я (U+0410..U+042F), 0xE0-0xEF = а-я (U+0430..U+044F),
            // 0xF0=Ё 0xF1=ё и др.; остальное — по таблице или fallback.
            uint16_t unicode;
            if (c >= 0x80 && c <= 0x9F)
                unicode = 0x0410 + (c - 0x80);
            else if (c >= 0xE0 && c <= 0xEF)
                unicode = 0x0430 + (c - 0xE0);
            else if (c == 0xF0) unicode = 0x0401;   // Ё
            else if (c == 0xF1) unicode = 0x0451;   // ё
            else
                unicode = 0x0400 + (c - 128);  // fallback для прочих символов CP866
            // UTF-8 для U+0080..U+07FF - 2 bytes
            utf8_str += (char)(0xC0 | ((unicode >> 6) & 0x1F));
            utf8_str += (char)(0x80 | (unicode & 0x3F));
        }
    }
    return utf8_str;
}

bn::string<CP866_STRING_MAX_LEN> cp866_to_utf8(const char* cp866_str) {
    return _cp866_to_utf8_impl(reinterpret_cast<const unsigned char*>(cp866_str));
}

bn::string<CP866_STRING_MAX_LEN> cp866_to_utf8(const unsigned char* cp866_str) {
    return _cp866_to_utf8_impl(cp866_str);
}

// UTF-8 Cyrillic (D0 90–AF, D1 80–9F, D0 81 Ё, D1 91 ё) -> CP866 (80–9F А–Я, A0–AF а–п, E0–EF р–я, F0 ё/Ё)
int utf_to_cp866(const char* utf8, unsigned char* out, int out_max) {
    int n = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8);
    while (n < out_max && *p) {
        if (*p < 0x80) {
            out[n++] = *p++;
            continue;
        }
        if (p[0] == 0xD0 && p[1] >= 0x90 && p[1] <= 0x9F) {
            out[n++] = (unsigned char)(0x80 + (p[1] - 0x90));
            p += 2;
            continue;
        }
        if (p[0] == 0xD0 && p[1] >= 0xA0 && p[1] <= 0xAF) {
            out[n++] = (unsigned char)(0x90 + (p[1] - 0xA0));
            p += 2;
            continue;
        }
        if (p[0] == 0xD0 && p[1] == 0x81) {
            out[n++] = 0xF0;  // Ё
            p += 2;
            continue;
        }
        if (p[0] == 0xD1 && p[1] >= 0x90 && p[1] <= 0x9F) {
            out[n++] = (unsigned char)(0xA0 + (p[1] - 0x90));
            p += 2;
            continue;
        }
        if (p[0] == 0xD1 && p[1] >= 0x80 && p[1] <= 0x8F) {
            out[n++] = (unsigned char)(0xE0 + (p[1] - 0x80));
            p += 2;
            continue;
        }
        if (p[0] == 0xD1 && p[1] == 0x91) {
            out[n++] = 0xF0;  // ё
            p += 2;
            continue;
        }
        ++p;
    }
    return n;
}

// CP866: a-z 0x61-0x7A -> 0x41-0x5A; а-п 0xA0-0xAF -> 0x80-0x8F; р-я 0xE0-0xEF -> 0x90-0x9F; ё 0xF0 -> Е 0x85
void cp866_upper_8(const unsigned char* in, unsigned char* out) {
    for (int i = 0; i < 8; ++i) {
        unsigned char c = in[i];
        if (c >= 0x61 && c <= 0x7A)
            c -= 0x20;
        else if (c >= 0xA0 && c <= 0xAF)
            c -= 0x20;
        else if (c >= 0xE0 && c <= 0xEF)
            c -= 0x50;
        else if (c == 0xF0)
            c = 0x85;
        out[i] = c;
    }
}