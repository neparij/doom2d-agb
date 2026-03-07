#!/usr/bin/env python3
"""
Замена графических лумпов в WAD на картинки из BMP.
Размер может не совпадать — подменяется весь lump (заголовок 8 байт + пиксели).

Формат lump: width LE16, height LE16, offset_x LE16, offset_y LE16, затем width*height байт (8-bit индексы палитры).
BMP: 8-bit индексированный, без RLE. Палитра не используется.

Usage:
  python wad_gfx_replace.py input.wad [output.wad]
"""

import struct
import sys
import os
from typing import Dict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from imports.wadfile import WadFile

# Лумп в WAD -> путь к BMP (относительно CWD или абсолютный).
REPLACE_MAP: Dict[str, str] = {
    "TITLEPIC": "../intermediate/TITLEPIC_GBA.bmp",
    "INTERPIC": "../intermediate/INTERPIC_GBA.bmp",
    "ENDPIC": "../intermediate/ENDPIC_GBA.bmp",
}


def load_bmp_as_lump(bmp_path: str) -> bytes:
    """
    Читает 8-bit BMP (индексированный, без RLE). Возвращает lump: 8 байт заголовок + пиксели.
    Палитра игнорируется.
    """
    with open(bmp_path, "rb") as f:
        data = f.read()

    if len(data) < 54:
        raise ValueError("BMP too small")
    if data[0:2] != b"BM":
        raise ValueError("Not a BMP file")

    pix_offset = struct.unpack("<I", data[10:14])[0]
    w = struct.unpack("<i", data[18:22])[0]
    h = struct.unpack("<i", data[22:26])[0]
    bpp = struct.unpack("<H", data[28:30])[0]
    compression = struct.unpack("<I", data[30:34])[0]

    if bpp != 8:
        raise ValueError("BMP must be 8-bit indexed")
    if compression != 0:
        raise ValueError("BMP must be uncompressed (no RLE)")

    if w <= 0 or h == 0:
        raise ValueError("Invalid BMP dimensions")

    h_abs = abs(h)
    row_bytes = ((w + 3) // 4) * 4
    if len(data) < pix_offset + row_bytes * h_abs:
        raise ValueError("BMP truncated")

    pixels = bytearray()
    if h > 0:
        for row in range(h_abs - 1, -1, -1):
            off = pix_offset + row * row_bytes
            pixels += data[off : off + w]
    else:
        for row in range(h_abs):
            off = pix_offset + row * row_bytes
            pixels += data[off : off + w]

    return struct.pack("<HHHH", w, h_abs, 0, 0) + bytes(pixels)


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: wad_gfx_replace.py <input.wad> [output.wad]", file=sys.stderr)
        sys.exit(1)
    in_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) >= 3 else in_path

    if not REPLACE_MAP:
        print("REPLACE_MAP is empty; nothing to replace.", file=sys.stderr)
        sys.exit(0)

    wad = WadFile.from_path(in_path)
    replaced = []

    for lump_name, gfx_path in REPLACE_MAP.items():
        if wad.find_lump(lump_name) is None:
            print(f"Lump {lump_name!r} not found in WAD, skip.", file=sys.stderr)
            continue
        if not os.path.isfile(gfx_path):
            print(f"File not found: {gfx_path}, skip.", file=sys.stderr)
            continue
        try:
            lump_data = load_bmp_as_lump(gfx_path)
        except Exception as e:
            print(f"Failed to load {gfx_path}: {e}", file=sys.stderr)
            continue
        wad = wad.replace_lump(lump_data, name=lump_name)
        replaced.append(f"{lump_name} <- {gfx_path}")

    wad.write(out_path)
    if replaced:
        print("Replaced:", "\n          ".join(replaced))
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()
