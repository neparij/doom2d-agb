#ifndef STCFN_FONT_SPRITE_FONT_H
#define STCFN_FONT_SPRITE_FONT_H

#include "bn_sprite_font.h"
#include "bn_sprite_items_stcfn_font.h"


#include "bn_utf8_characters_map.h"

constexpr bn::utf8_character stcfn_font_sprite_font_utf8_characters[] = {
    "А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я"
};

constexpr bn::span<const bn::utf8_character> stcfn_font_sprite_font_utf8_characters_span(
        stcfn_font_sprite_font_utf8_characters);

constexpr auto stcfn_font_sprite_font_utf8_characters_map =
        bn::utf8_characters_map<stcfn_font_sprite_font_utf8_characters_span>();

constexpr int8_t stcfn_font_sprite_font_character_widths[] = {
    4,  // Space
    4,  // !
    7,  // "
    7,  // #
    7,  // $
    9,  // %
    8,  // &
    4,  // '
    7,  // (
    7,  // )
    7,  // *
    5,  // +
    4,  // ,
    6,  // -
    4,  // .
    7,  // /
    8,  // 0
    5,  // 1
    8,  // 2
    8,  // 3
    7,  // 4
    7,  // 5
    8,  // 6
    8,  // 7
    8,  // 8
    8,  // 9
    4,  // :
    4,  // ;
    5,  // <
    5,  // =
    5,  // >
    8,  // ?
    9,  // @
    8,  // A
    8,  // B
    8,  // C
    8,  // D
    8,  // E
    8,  // F
    8,  // G
    8,  // H
    4,  // I
    8,  // J
    8,  // K
    8,  // L
    9,  // M
    8,  // N
    8,  // O
    8,  // P
    8,  // Q
    8,  // R
    7,  // S
    8,  // T
    8,  // U
    7,  // V
    9,  // W
    9,  // X
    8,  // Y
    7,  // Z
    5,  // [
    7,  // Backslash
    5,  // ]
    7,  // ^
    8,  // _
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
    8,  // А
    8,  // Б
    8,  // В
    8,  // Г
    10,  // Д
    8,  // Е
    12,  // Ж
    8,  // З
    8,  // И
    8,  // Й
    8,  // К
    8,  // Л
    9,  // М
    8,  // Н
    8,  // О
    8,  // П
    8,  // Р
    8,  // С
    8,  // Т
    7,  // У
    10,  // Ф
    9,  // Х
    9,  // Ц
    7,  // Ч
    10,  // Ш
    11,  // Щ
    10,  // Ъ
    11,  // Ы
    8,  // Ь
    8,  // Э
    11,  // Ю
    8,  // Я
};

constexpr bn::sprite_font stcfn_font_sprite_font(
        bn::sprite_items::stcfn_font,
        stcfn_font_sprite_font_utf8_characters_map.reference(),
        stcfn_font_sprite_font_character_widths);

#endif