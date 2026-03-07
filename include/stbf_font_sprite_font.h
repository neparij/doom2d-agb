#ifndef STBF_FONT_SPRITE_FONT_H
#define STBF_FONT_SPRITE_FONT_H

#include "bn_sprite_font.h"
#include "bn_sprite_items_stbf_font.h"


#include "bn_utf8_characters_map.h"

constexpr bn::utf8_character stbf_font_sprite_font_utf8_characters[] = {
    "А", "Б", "В", "Г", "Д", "Е", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я", "а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м", "н", "о", "п", "р", "с", "т", "у", "ф", "х", "ц", "ч", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я"
};

constexpr bn::span<const bn::utf8_character> stbf_font_sprite_font_utf8_characters_span(
        stbf_font_sprite_font_utf8_characters);

constexpr auto stbf_font_sprite_font_utf8_characters_map =
        bn::utf8_characters_map<stbf_font_sprite_font_utf8_characters_span>();

// forced_width: широкие буквы режутся, хвост → lowercase:
//   O (16px) + o (1px)
//   Q (16px) + q (1px)
//   Д (16px) + д (1px)
//   Ж (16px) + ж (10px)
//   О (16px) + о (1px)
//   Щ (16px) + щ (1px)
//   Ъ (16px) + ъ (1px)
//   Ы (16px) + ы (3px)
//   Ю (16px) + ю (6px)

constexpr int8_t stbf_font_sprite_font_character_widths[] = {
    4,  // Space
    4,  // !
    4,  // "
    4,  // #
    4,  // $
    4,  // %
    4,  // &
    5,  // '
    4,  // (
    4,  // )
    4,  // *
    4,  // +
    4,  // ,
    4,  // -
    4,  // .
    4,  // /
    4,  // 0
    4,  // 1
    4,  // 2
    4,  // 3
    4,  // 4
    4,  // 5
    4,  // 6
    4,  // 7
    4,  // 8
    4,  // 9
    4,  // :
    4,  // ;
    4,  // <
    4,  // =
    4,  // >
    4,  // ?
    4,  // @
    15,  // A
    15,  // B
    14,  // C
    15,  // D
    14,  // E
    12,  // F
    16,  // G
    15,  // H
    6,  // I
    11,  // J
    16,  // K
    11,  // L
    16,  // M
    16,  // N
    16,  // O
    14,  // P
    16,  // Q
    15,  // R
    15,  // S
    12,  // T
    16,  // U
    16,  // V
    16,  // W
    14,  // X
    16,  // Y
    16,  // Z
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
    1,  // o
    4,  // p
    1,  // q
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
    15,  // А
    15,  // Б
    15,  // В
    11,  // Г
    16,  // Д
    14,  // Е
    16,  // Ж
    13,  // З
    16,  // И
    16,  // Й
    16,  // К
    15,  // Л
    16,  // М
    15,  // Н
    16,  // О
    15,  // П
    14,  // Р
    14,  // С
    12,  // Т
    15,  // У
    4,  // Ф
    14,  // Х
    16,  // Ц
    15,  // Ч
    16,  // Ш
    16,  // Щ
    16,  // Ъ
    16,  // Ы
    14,  // Ь
    14,  // Э
    16,  // Ю
    16,  // Я
    4,  // а
    4,  // б
    4,  // в
    4,  // г
    1,  // д
    4,  // е
    10,  // ж
    4,  // з
    4,  // и
    4,  // й
    4,  // к
    4,  // л
    4,  // м
    4,  // н
    1,  // о
    4,  // п
    4,  // р
    4,  // с
    4,  // т
    4,  // у
    4,  // ф
    4,  // х
    4,  // ц
    4,  // ч
    4,  // ш
    1,  // щ
    1,  // ъ
    3,  // ы
    4,  // ь
    4,  // э
    6,  // ю
    4,  // я
};

constexpr bn::sprite_font stbf_font_sprite_font(
        bn::sprite_items::stbf_font,
        stbf_font_sprite_font_utf8_characters_map.reference(),
        stbf_font_sprite_font_character_widths);

#endif