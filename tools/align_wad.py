#!/usr/bin/env python3
"""
Выравнивает лумпы в WAD по 4 байта.
Читает WAD, перезаписывает с паддингом между лумпами так, чтобы адрес каждого лумпа
в файле был кратен 4. Таблица директории обновляется с новыми адресами.

Usage:
  python align_wad.py input.wad [output.wad]
  Если output не указан — перезаписывает input.wad.
"""

import struct
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from imports.wadfile import WadFile

ALIGN = 4


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: align_wad.py <input.wad> [output.wad]", file=sys.stderr)
        sys.exit(1)
    in_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) >= 3 else in_path

    wad = WadFile.from_path(in_path)
    lumps = wad.to_lumps()

    pos = WadFile.HEADER_SIZE
    dir_entries = []
    data_parts = []

    for name_8, payload in lumps:
        pad = (ALIGN - pos % ALIGN) % ALIGN
        pos += pad
        new_filepos = pos

        data_parts.append((pad, payload))
        dir_entries.append((new_filepos, len(payload), name_8))
        pos += len(payload)

    dir_offset = pos

    out = bytearray()
    out += wad.id
    out += struct.pack("<I", len(lumps))
    out += struct.pack("<I", dir_offset)
    for pad, payload in data_parts:
        out += b"\x00" * pad
        out += payload
    for fp, sz, name_8 in dir_entries:
        out += struct.pack("<II", fp, sz)
        out += name_8

    with open(out_path, "wb") as f:
        f.write(out)

    print(f"Aligned {len(lumps)} lumps to {ALIGN} bytes, wrote to {out_path}")


if __name__ == "__main__":
    main()
