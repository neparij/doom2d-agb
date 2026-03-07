#!/usr/bin/env python3
"""
Добавляет в WAD «невыровненные» версии лумпов неба: картинка сдвинута на 1 пиксель влево,
чтобы при копировании по 16/32 бит с выровненными адресами отображение было корректным.

Формат лумпов неба (как в Doom2D): 8 байт заголовок (width LE16, height LE16, offset_x LE16, offset_y LE16),
далее width*height байт пикселей (row-major).

Usage:
  python add_unaligned_skies.py input.wad [output.wad]
"""

import struct
import sys
import os
from typing import List, Tuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from imports.wadfile import WadFile

# Исходный lump -> имя нового lump (unaligned). Можно расширить.
SKY_ALIGN_MAP = {
    "RSKY1": "RSKYU1",
    "RSKY2": "RSKYU2",
}

HEADER_SIZE = 8


def parse_sky_lump(data: bytes) -> tuple:
    """(width, height, offset_x, offset_y, pixels_bytes)."""
    if len(data) < HEADER_SIZE:
        raise ValueError(f"Lump too small: {len(data)} < {HEADER_SIZE}")
    w, h = struct.unpack("<HH", data[0:4])
    ox, oy = struct.unpack("<HH", data[4:8])
    expected = w * h
    if len(data) != HEADER_SIZE + expected:
        raise ValueError(f"Lump size mismatch: have {len(data) - HEADER_SIZE}, need w*h={expected}")
    return (w, h, ox, oy, data[HEADER_SIZE:])


def shift_row_left(pixels: bytes, width: int) -> bytes:
    """Сдвиг одной строки на 1 пиксель влево (с wrap: первый пиксель = бывший второй)."""
    if width <= 0:
        return pixels
    return pixels[1:width] + bytes([pixels[0]])


def build_sky_shifted_left(data: bytes) -> bytes:
    """Новый lump: тот же заголовок, пиксели сдвинуты на 1 влево по каждой строке."""
    w, h, ox, oy, pixels = parse_sky_lump(data)
    new_pixels = bytearray()
    for row in range(h):
        row_start = row * w
        row_data = pixels[row_start : row_start + w]
        new_pixels += shift_row_left(row_data, w)
    return data[0:HEADER_SIZE] + bytes(new_pixels)


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: add_unaligned_skies.py <input.wad> [output.wad]", file=sys.stderr)
        sys.exit(1)
    in_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) >= 3 else in_path

    wad = WadFile.from_path(in_path)
    lumps = wad.to_lumps()
    new_lumps: List[Tuple[bytes, bytes]] = []
    added = []

    for name_8, payload in lumps:
        new_lumps.append((name_8, payload))
        name_str = WadFile.decode_lump_name(name_8)
        if name_str in SKY_ALIGN_MAP:
            dst_name = SKY_ALIGN_MAP[name_str]
            try:
                shifted = build_sky_shifted_left(payload)
            except Exception as e:
                print(f"Skip {name_str} -> {dst_name}: {e}", file=sys.stderr)
                continue
            dst_8 = WadFile._pack_name(dst_name)
            new_lumps.append((dst_8, shifted))
            added.append(f"{name_str} -> {dst_name}")

    out_wad = WadFile.from_lumps(wad.id, new_lumps)
    out_wad.write(out_path)
    if added:
        print(f"Added unaligned: {', '.join(added)}")
    print(f"Wrote {len(new_lumps)} lumps to {out_path}")


if __name__ == "__main__":
    main()
