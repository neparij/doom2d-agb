#!/usr/bin/env python3
"""
Генерация спрайт-шрифтов из графических лумпов WAD для Butano.

- Читает WAD, находит PLAYPAL (палитра 256 цветов × 3 байта RGB)
- Собирает лумпы по chars_str + char_to_lump: для каждого символа получает имя лумпа
- Строит укороченную палитру (16 цветов): 0 всегда первый, остальные — 15 самых используемых
- Собирает вертикальную полоску тайлов, записывает BMP + JSON (Butano) + C++ заголовок

Butano: sprite_font с utf8_characters_map и character_widths (variable width).
Порядок тайлов = порядок символов в chars_str. Первый символ — пробел (пустой тайл).

Usage:
  python generate_sprite_fonts.py input.wad
"""

import json
import struct
import sys
import os
from typing import Callable, List, Optional, Tuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from imports.wadfile import WadFile

GFX_HEADER_SIZE = 8
PLAYPAL_SIZE = 256 * 3  # 256 colors, 3 bytes RGB each
MAX_SHORT_PAL = 16

_BASIC_ASCII = "".join(chr(i) for i in range(0x21, 0x7F)) # Printable ASCII (except space)
_DIGITS_ASCII = "".join(chr(i) for i in range(0x30, 0x3A)) # 0-9
_RUSSIAN_UPPERCASE = "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
_RUSSIAN_LOWERCASE = "абвгдежзийклмнопрстуфхцчшщъыьэюя"


def parse_gfx_lump(data: bytes) -> Tuple[int, int, int, int, bytes]:
    """(width, height, offset_x, offset_y, pixels)."""
    if len(data) < GFX_HEADER_SIZE:
        raise ValueError(f"Lump too small: {len(data)}")
    w, h = struct.unpack("<HH", data[0:4])
    ox, oy = struct.unpack("<hh", data[4:8])  # signed: leftoffset, topoffset
    expected = w * h
    if len(data) != GFX_HEADER_SIZE + expected:
        raise ValueError(f"Lump size mismatch: {len(data)} != 8 + {w}*{h}")
    return (w, h, ox, oy, data[GFX_HEADER_SIZE:])


def _crop_lump_horizontal(
    lump: Tuple[int, int, int, int, bytes], start_x: int, crop_w: int
) -> Tuple[int, int, int, int, bytes]:
    """Вырезает горизонтальный срез из лумпа. (w, h, ox, oy, pixels)."""
    w, h, ox, oy, pixels = lump
    if start_x < 0 or crop_w <= 0 or start_x + crop_w > w:
        raise ValueError(f"Invalid crop: w={w}, start_x={start_x}, crop_w={crop_w}")
    cropped = bytearray(crop_w * h)
    for row in range(h):
        src = pixels[row * w + start_x : row * w + start_x + crop_w]
        cropped[row * crop_w : row * crop_w + crop_w] = src
    return (crop_w, h, 0, oy, bytes(cropped))


def _effective_width(w: int, ox: int) -> int:
    """Ширина с учётом отрицательного ox: при ox < 0 добавляем |ox| для корректного отображения."""
    return w - ox if ox < 0 else w


def _color_dist_sq(playpal: bytes, i: int, j: int) -> int:
    """Квадрат расстояния между цветами i и j в PLAYPAL (RGB)."""
    r = playpal[i * 3] - playpal[j * 3]
    g = playpal[i * 3 + 1] - playpal[j * 3 + 1]
    b = playpal[i * 3 + 2] - playpal[j * 3 + 2]
    return r * r + g * g + b * b


def build_short_palette_reduced(
    used_counts: dict,
    playpal: bytes,
) -> Tuple[List[int], dict]:
    """
    Строит укороченную палитру из 16 цветов: 0 всегда первый, остальные — 15 самых используемых.
    Остальные индексы маппятся на ближайший по RGB. Возвращает (short_pal, old_to_new).
    """
    short_pal = [0]
    others = [(cnt, idx) for idx, cnt in used_counts.items() if idx != 0]
    others.sort(key=lambda x: (-x[0], x[1]))
    for _, idx in others[: MAX_SHORT_PAL - 1]:
        short_pal.append(idx)

    old_to_new = {}
    for new_idx, old_idx in enumerate(short_pal):
        old_to_new[old_idx] = new_idx

    for old_idx in range(256):
        if old_idx in old_to_new:
            continue
        best_new, best_dist = 0, 10**9
        for new_idx, pal_idx in enumerate(short_pal):
            d = _color_dist_sq(playpal, old_idx, pal_idx)
            if d < best_dist:
                best_dist, best_new = d, new_idx
        old_to_new[old_idx] = best_new

    return (short_pal, old_to_new)


def write_bmp_8bit(
    path: str,
    width: int,
    height: int,
    pixels: bytes,
    palette_rgb: List[Tuple[int, int, int]],
) -> None:
    """Пишет 8-bit BMP без RLE. palette_rgb — до 256 цветов (B,G,R)."""
    while len(palette_rgb) < 256:
        palette_rgb.append((0, 0, 0))
    row_bytes = ((width + 3) // 4) * 4
    pix_size = row_bytes * height
    pix_offset = 14 + 40 + 256 * 4
    file_size = pix_offset + pix_size

    with open(path, "wb") as f:
        f.write(b"BM")
        f.write(struct.pack("<I", file_size))
        f.write(struct.pack("<HH", 0, 0))
        f.write(struct.pack("<I", pix_offset))
        f.write(struct.pack("<I", 40))
        f.write(struct.pack("<ii", width, -height))
        f.write(struct.pack("<HH", 1, 8))
        f.write(struct.pack("<I", 0))
        f.write(struct.pack("<I", pix_size))
        f.write(struct.pack("<ii", 0, 0))
        f.write(struct.pack("<II", 256, 256))
        for b, g, r in palette_rgb[:256]:
            f.write(struct.pack("BBBB", b, g, r, 0))
        for row in range(height):
            f.write(pixels[row * width : (row + 1) * width])
            f.write(b"\x00" * (row_bytes - width))


def write_bmp_4bpp(
    path: str,
    width: int,
    height: int,
    pixels: bytes,
    palette_rgb: List[Tuple[int, int, int]],
) -> None:
    """Пишет 4-bit BMP (16 цветов) для Butano sprite_font (BPP_4).
    Высота положительная (bottom-up): Butano BMP reader читает height как unsigned.
    """
    assert len(palette_rgb) <= 16
    while len(palette_rgb) < 16:
        palette_rgb.append((0, 0, 0))
    row_bytes = ((width // 2 + 3) // 4) * 4
    pix_size = row_bytes * height
    pix_offset = 14 + 40 + 16 * 4
    file_size = pix_offset + pix_size

    with open(path, "wb") as f:
        f.write(b"BM")
        f.write(struct.pack("<I", file_size))
        f.write(struct.pack("<HH", 0, 0))
        f.write(struct.pack("<I", pix_offset))
        f.write(struct.pack("<I", 40))
        f.write(struct.pack("<ii", width, height))  # положительная высота (bottom-up)
        f.write(struct.pack("<HH", 1, 4))
        f.write(struct.pack("<I", 0))
        f.write(struct.pack("<I", pix_size))
        f.write(struct.pack("<ii", 0, 0))
        f.write(struct.pack("<II", 16, 16))
        for b, g, r in palette_rgb[:16]:
            f.write(struct.pack("BBBB", b, g, r, 0))
        for row in range(height - 1, -1, -1):  # bottom-up: последняя строка первой
            row_pixels = pixels[row * width : (row + 1) * width]
            packed = bytearray(row_bytes)
            for col in range(0, width, 2):
                p0 = row_pixels[col] & 0x0F
                p1 = row_pixels[col + 1] & 0x0F if col + 1 < width else 0
                packed[col // 2] = (p0 << 4) | p1
            f.write(packed)


def _escape_cpp_string(s: str) -> str:
    """Экранирование для C++ строкового литерала."""
    result = []
    for c in s:
        if c == "\\":
            result.append("\\\\")
        elif c == '"':
            result.append('\\"')
        elif c == "\n":
            result.append("\\n")
        elif c == "\t":
            result.append("\\t")
        else:
            result.append(c)
    return "".join(result)


# Конфигурация Butano-шрифта:
# - output_base: базовое имя (stbf_font) — получим stbf_font.bmp, stbf_font.json, stbf_font_sprite_font.h
# - output_dir: папка для BMP и JSON (graphics)
# - header_path: путь к .h (include/stbf_font_sprite_font.h)
# - filter_fn: (name, index) -> bool — какие лумпы учитывать из WAD
# - chars_str: строка символов в порядке тайлов (первый = пробел)
# - char_to_lump_fn: (char: str) -> Optional[str] — имя лумпа для символа; None/"" = пустой тайл (пробел)
# - tile_w, tile_h, default_h: размер тайла и базовая высота
# - namespace: опционально, напр. "common"
# - forced_width: если символ шире — левая часть на основной символ, правая на lowercase
SpriteFontConfig = Tuple[
    str,  # output_base
    str,  # output_dir
    str,  # header_path
    Callable[[str, int], bool],  # filter_fn
    str,  # chars_str
    Callable[[str], Optional[str]],  # char_to_lump_fn
    int,  # tile_w
    int,  # tile_h
    int,  # default_h
    Optional[str],  # namespace
    Optional[int],  # forced_width
]

SPRITE_FONT_CONFIGS: List[SpriteFontConfig] = [
    (
        "stbf_font",
        "graphics",
        "include/stbf_font_sprite_font.h",
        lambda n, i: n.startswith("STBF_") and n != "STBF_.",
        _BASIC_ASCII + _RUSSIAN_UPPERCASE + _RUSSIAN_LOWERCASE,
        lambda c: None if c == " " else f"STBF_{c}",
        16,
        16,
        12,
        None,
        16,  # forced_width: широкие буквы (Ж, Д, Ю и т.д.) режутся, хвост → lowercase
    ),
    (
        "stcfn_font",
        "graphics",
        "include/stcfn_font_sprite_font.h",
        lambda n, i: n.startswith("STCFN") and len(n) == 8,
        _BASIC_ASCII + _RUSSIAN_UPPERCASE,
        lambda c: f"STCFN{c.encode('cp866')[0]:03d}",  # ASCII + CP866
        16,
        16,
        7,
        None,
        None,
    ),
    (
        "wi_font",
        "graphics",
        "include/wi_font_sprite_font.h",
        lambda n, i: n.startswith("WI") and (n.startswith("WINUM") or n in ("WIMINUS", "WIPCNT", "WICOLON")) or n == "STBF_.",
        _BASIC_ASCII,
        lambda c: (
            f"WINUM{c}" if c in _DIGITS_ASCII else
            "WIPCNT" if c == "%" else
            "WIMINUS" if c == "-" else
            "WICOLON" if c == ":" else
            "STBF_." if c == "." else
            None
        ),
        16,
        16,
        12,
        None,
        None,
    ),
    (
        "stt_font",
        "graphics",
        "include/stt_font_sprite_font.h",
        lambda n, i: n.startswith("STT") and (n.startswith("STTNUM") or n in ("STTMINUS", "STTPRCNT")),
        _BASIC_ASCII,
        lambda c: (
            f"STTNUM{c}" if c in _DIGITS_ASCII else
            "STTPRCNT" if c == "%" else
            "STTMINUS" if c == "-" else
            None
        ),
        16,
        16,
        16,
        None,
        None,
    ),
]


def generate_one(
    wad: WadFile,
    playpal: bytes,
    config: SpriteFontConfig,
) -> None:
    (
        output_base,
        output_dir,
        header_path,
        filter_fn,
        chars_str,
        char_to_lump_fn,
        tile_w,
        tile_h,
        default_h,
        namespace,
        forced_width,
    ) = config

    # Собираем лумпы из WAD по фильтру
    lump_by_name: dict = {}
    for i in range(wad.num_lumps):
        name = wad.get_lump_name(i)
        if not filter_fn(name, i):
            continue
        try:
            data = wad.get_lump_data(i)
            w, h, ox, oy, pixels = parse_gfx_lump(data)
            lump_by_name[name] = (w, h, ox, oy, pixels)
        except Exception as e:
            print(f"Skip lump {name}: {e}", file=sys.stderr)
            continue

    # При forced_width: continuation_targets[c_lower] = (lump, start_x, cont_width)
    continuation_targets: dict = {}
    split_pairs: List[Tuple[str, str, int, int]] = []
    if forced_width is not None:
        for c in chars_str:
            lump_name = char_to_lump_fn(c)
            if not lump_name or lump_name not in lump_by_name:
                continue
            lump = lump_by_name[lump_name]
            w = lump[0]
            if w > forced_width and c.lower() != c:
                c_lower = c.lower()
                if c_lower in continuation_targets:
                    print(
                        f"Warning [{output_base}]: char {repr(c_lower)} already targeted by "
                        f"another split, now overwritten by {repr(c)}",
                        file=sys.stderr,
                    )
                continuation_targets[c_lower] = (lump, forced_width, w - forced_width)
                split_pairs.append((c, c_lower, forced_width, w - forced_width))
        if split_pairs:
            print(f"[{output_base}] forced_width={forced_width}, split letters:")
            for uc, lc, w_main, w_cont in split_pairs:
                print(f"  {uc} ({w_main}px) + {lc} ({w_cont}px)")

    # Порядок тайлов по chars_str
    mapped_chars: set = set()  # символы, которым уже назначен тайл (оригинал или continuation)
    ordered_lumps: List[Tuple[str, Optional[Tuple[int, int, int, int, bytes]], int]] = []
    for c in chars_str:
        lump_name = char_to_lump_fn(c)
        width_override: Optional[int] = None

        if c in continuation_targets:
            # Правый хвост широкого символа → lowercase
            lump, start_x, cont_w = continuation_targets[c]
            lump = _crop_lump_horizontal(lump, start_x, cont_w)
            width_override = cont_w
            if lump_name and lump_name in lump_by_name:
                print(
                    f"Warning [{output_base}]: char {repr(c)} gets continuation from uppercase, "
                    f"but own lump {lump_name} exists — overwriting",
                    file=sys.stderr,
                )
            if c in mapped_chars:
                print(
                    f"Warning [{output_base}]: char {repr(c)} already mapped",
                    file=sys.stderr,
                )
            mapped_chars.add(c)
            ordered_lumps.append((c, lump, width_override))
        elif not lump_name:
            ordered_lumps.append((c, None, None))
        elif lump_name in lump_by_name:
            lump = lump_by_name[lump_name]
            w, h, ox, oy, pixels = lump
            if forced_width is not None and w > forced_width and c.lower() != c:
                # Левый кусок → основной символ
                lump = _crop_lump_horizontal(lump, 0, forced_width)
                width_override = forced_width
            else:
                width_override = _effective_width(w, ox)
            if c in mapped_chars:
                print(
                    f"Warning [{output_base}]: char {repr(c)} already mapped",
                    file=sys.stderr,
                )
            mapped_chars.add(c)
            ordered_lumps.append((c, lump, width_override))
        else:
            print(f"Warning: no lump for char {repr(c)} (expected {lump_name})", file=sys.stderr)
            ordered_lumps.append((c, None, None))

    # Собираем только те лумпы, что есть
    lumps_data = [ld for _, ld, _ in ordered_lumps if ld is not None]
    if not lumps_data:
        print(f"No lumps for {output_base}, skip.", file=sys.stderr)
        return

    used_counts: dict = {}
    for _, _, _, _, px in lumps_data:
        for p in px:
            used_counts[p] = used_counts.get(p, 0) + 1

    short_pal, old_to_new = build_short_palette_reduced(used_counts, playpal)

    palette_rgb = []
    color_channel_multiplier = 4
    for idx in short_pal:
        r, g, b = playpal[idx * 3], playpal[idx * 3 + 1], playpal[idx * 3 + 2]
        r = r * color_channel_multiplier
        g = g * color_channel_multiplier
        b = b * color_channel_multiplier
        palette_rgb.append((b, g, r))

    out_pixels = bytearray()
    character_widths: List[int] = []

    for c, lump, width_override in ordered_lumps:
        tile_buf = bytearray(tile_w * tile_h)
        if lump is not None:
            w, h, ox, oy, pixels = lump
            character_widths.append(width_override if width_override is not None else w)
            pad_x = max(1 - w, min(tile_w - 1, -ox))
            pad_y = max(1 - h, min(tile_h - 1, (tile_h - default_h) // 2 - oy))
            for row in range(tile_h):
                for col in range(tile_w):
                    src_x = col - pad_x
                    src_y = row - pad_y
                    if 0 <= src_x < w and 0 <= src_y < h:
                        old_idx = pixels[src_y * w + src_x]
                        tile_buf[row * tile_w + col] = old_to_new[old_idx]
        else:
            character_widths.append(tile_w // 4)  # пробел
        out_pixels.extend(tile_buf)

    out_w = tile_w
    out_h = len(ordered_lumps) * tile_h

    # Разделение на ASCII и UTF-8
    ascii_chars = []
    utf8_chars = []
    for c in chars_str:
        if ord(c) < 0x80:
            ascii_chars.append(c)
        else:
            utf8_chars.append(c)

    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(os.path.dirname(header_path) or ".", exist_ok=True)

    bmp_path = os.path.join(output_dir, f"{output_base}.bmp")
    json_path = os.path.join(output_dir, f"{output_base}.json")

    write_bmp_4bpp(bmp_path, out_w, out_h, bytes(out_pixels), palette_rgb)
    print(f"Wrote {bmp_path} ({len(ordered_lumps)} tiles)")

    with open(json_path, "w", encoding="utf-8") as f:
        json.dump({"type": "sprite", "height": tile_h}, f, indent=4)
    print(f"Wrote {json_path}")

    # C++ заголовок
    sprite_items_name = f"bn_sprite_items_{output_base}"
    font_var_name = output_base.replace("-", "_") + "_sprite_font"
    guard = (output_base.upper().replace("-", "_") + "_SPRITE_FONT_H").replace(" ", "_")

    ns_open = f"namespace {namespace} {{\n\n" if namespace else ""
    ns_close = f"\n}}\n\n#endif" if namespace else "\n#endif"

    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "bn_sprite_font.h"',
        f'#include "{sprite_items_name}.h"',
        "",
        ns_open,
    ]

    if utf8_chars:
        lines.append('#include "bn_utf8_characters_map.h"')
        lines.append("")
        utf8_arr = ", ".join(f'"{_escape_cpp_string(c)}"' for c in utf8_chars)
        lines.append(f"constexpr bn::utf8_character {font_var_name}_utf8_characters[] = {{")
        lines.append(f"    {utf8_arr}")
        lines.append("};")
        lines.append("")
        lines.append(
            f"constexpr bn::span<const bn::utf8_character> {font_var_name}_utf8_characters_span("
        )
        lines.append(f"        {font_var_name}_utf8_characters);")
        lines.append("")
        lines.append(f"constexpr auto {font_var_name}_utf8_characters_map =")
        lines.append(f"        bn::utf8_characters_map<{font_var_name}_utf8_characters_span>();")
        lines.append("")

    # character_widths
    if split_pairs:
        lines.append("// forced_width: широкие буквы режутся, хвост → lowercase:")
        for uc, lc, w_main, w_cont in split_pairs:
            lines.append(f"//   {uc} ({w_main}px) + {lc} ({w_cont}px)")
        lines.append("")
    width_lines = [f"    {tile_w // 4},  // Space"]
    for i, (c, w) in enumerate(zip(chars_str, character_widths)):
        if c == "\\":
            comment = "Backslash"
        elif c == " ":
            comment = "Space"
        else:
            comment = c
        width_lines.append(f"    {w},  // {comment}")
    lines.append(f"constexpr int8_t {font_var_name}_character_widths[] = {{")
    lines.extend(width_lines)
    lines.append("};")
    lines.append("")

    lines.append(f"constexpr bn::sprite_font {font_var_name}(")
    lines.append(f"        bn::sprite_items::{output_base},")
    if utf8_chars:
        lines.append(f"        {font_var_name}_utf8_characters_map.reference(),")
    else:
        lines.append("        bn::utf8_characters_map_ref(),")
    lines.append(f"        {font_var_name}_character_widths);")

    lines.append(ns_close)

    with open(header_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"Wrote {header_path}")


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: generate_sprite_fonts.py <input.wad>", file=sys.stderr)
        sys.exit(1)
    wad_path = sys.argv[1]

    wad = WadFile.from_path(wad_path)
    playpal_result = wad.find_lump("PLAYPAL")
    if playpal_result is None:
        print("PLAYPAL not found in WAD", file=sys.stderr)
        sys.exit(1)
    _, playpal_data = playpal_result
    if len(playpal_data) < PLAYPAL_SIZE:
        print("PLAYPAL too small", file=sys.stderr)
        sys.exit(1)
    playpal = playpal_data[:PLAYPAL_SIZE]

    if not SPRITE_FONT_CONFIGS:
        print("SPRITE_FONT_CONFIGS is empty.", file=sys.stderr)
        sys.exit(0)

    for config in SPRITE_FONT_CONFIGS:
        try:
            generate_one(wad, playpal, config)
        except Exception as e:
            print(f"Error for {config[0]}: {e}", file=sys.stderr)
            raise


if __name__ == "__main__":
    main()
