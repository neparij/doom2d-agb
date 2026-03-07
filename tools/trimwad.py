#!/usr/bin/env python3
"""
Trim a WAD by removing lumps that match any of the given predicates.
Reads a WAD file and writes a new WAD with the same structure but without
the specified lumps.

Usage:
  python trimwad.py input.wad [output.wad]
  If output.wad is omitted, writes to input_trimmed.wad (or similar).
"""

import re
import sys
import os
from typing import Callable, List

# Add parent so imports.wadfile is findable when run from project root or tools/
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from imports.wadfile import WadFile


# Список предикатов: ламп удаляется, если хотя бы один вернул True для его имени (Unicode).
# Примеры:
#   lambda name: name.upper() in ("DEMO1", "DEMO2")
#   lambda name: bool(re.fullmatch(r"DMI\d{4}", name))
LUMPS_REMOVE_IF: List[Callable[[str], bool]] = [
    lambda name: bool(name in ("CD1PIC")),
    lambda name: name in (
        "ALLRIGHT",
        "BEST",
        "GLAD",
        "MENU",
        "INTERMUS",
        "SUPER",
        "АС",
        "АТАС",
        "АЙОЙ",
        "БОЖ_МУЗ",
        "БЫСТРОТА",
        "С_ВЫШАК",
        "КЛАСС",
        "ДУШЕВНАЯ",
        "ХЕХЕХЕ",
        "ХОХОХО",
        "КОНЕЦ",
        "МОЩЬ",
        "ПАЛАТА#6",
        "ПРИКОЛ",
        "ПРОБА",
        "ПРОСТОТА",
        "СОЛО",
        "СПОК",
        "СТРАХ",
        "ТИШЬ",
        "ТЫ_ПОБЕД",
        "ТЫ_ТРУП",
        "УЖАС",
        "ВЗБУЧКА"
    ),
    lambda name: bool(re.fullmatch(r"DMI\d{4}", name)),
    lambda name: bool(name.startswith("DS")),
]


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: trimwad.py <input.wad> [output.wad]", file=sys.stderr)
        sys.exit(1)
    in_path = sys.argv[1]
    if len(sys.argv) >= 3:
        out_path = sys.argv[2]
    else:
        base, ext = os.path.splitext(in_path)
        out_path = base + "_trimmed" + ext

    if not LUMPS_REMOVE_IF:
        print("LUMPS_REMOVE_IF is empty; output WAD will be identical to input.", file=sys.stderr)

    wad = WadFile.from_path(in_path)
    lumps = wad.to_lumps()

    def keep_lump(name_8: bytes, _data: bytes) -> bool:
        name_str = WadFile.decode_lump_name(name_8)
        return not any(pred(name_str) for pred in LUMPS_REMOVE_IF)

    filtered = [(n, d) for n, d in lumps if keep_lump(n, d)]
    removed_count = len(lumps) - len(filtered)

    out_wad = WadFile.from_lumps(wad.id, filtered)
    out_wad.write(out_path)
    print(f"Removed {removed_count} lump(s), wrote {len(filtered)} lumps to {out_path}")


if __name__ == "__main__":
    main()
