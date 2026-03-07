#ifndef STT_FONT_SPRITE_FONT_H
#define STT_FONT_SPRITE_FONT_H

#include "bn_sprite_font.h"
#include "bn_sprite_items_stt_font.h"


constexpr int8_t stt_font_sprite_font_character_widths[] = {
    4,  // Space
    4,  // !
    4,  // "
    4,  // #
    4,  // $
    14,  // %
    4,  // &
    4,  // '
    4,  // (
    4,  // )
    4,  // *
    4,  // +
    4,  // ,
    8,  // -
    4,  // .
    4,  // /
    14,  // 0
    12,  // 1
    14,  // 2
    14,  // 3
    14,  // 4
    14,  // 5
    14,  // 6
    14,  // 7
    14,  // 8
    14,  // 9
    4,  // :
    4,  // ;
    4,  // <
    4,  // =
    4,  // >
    4,  // ?
    4,  // @
    4,  // A
    4,  // B
    4,  // C
    4,  // D
    4,  // E
    4,  // F
    4,  // G
    4,  // H
    4,  // I
    4,  // J
    4,  // K
    4,  // L
    4,  // M
    4,  // N
    4,  // O
    4,  // P
    4,  // Q
    4,  // R
    4,  // S
    4,  // T
    4,  // U
    4,  // V
    4,  // W
    4,  // X
    4,  // Y
    4,  // Z
    4,  // [
    4,  // Backslash
    4,  // ]
    4,  // ^
    4,  // _
    4,  // `
    4,  // a
    4,  // b
    4,  // c
    4,  // d
    4,  // e
    4,  // f
    4,  // g
    4,  // h
    4,  // i
    4,  // j
    4,  // k
    4,  // l
    4,  // m
    4,  // n
    4,  // o
    4,  // p
    4,  // q
    4,  // r
    4,  // s
    4,  // t
    4,  // u
    4,  // v
    4,  // w
    4,  // x
    4,  // y
    4,  // z
    4,  // {
    4,  // |
    4,  // }
    4,  // ~
};

constexpr bn::sprite_font stt_font_sprite_font(
        bn::sprite_items::stt_font,
        bn::utf8_characters_map_ref(),
        stt_font_sprite_font_character_widths);

#endif