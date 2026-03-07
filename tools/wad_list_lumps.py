#!/usr/bin/env python3
"""
Print a list of all lumps in a WAD file.
Names are decoded as CP866 (Cyrillic supported).

Usage:
  python wad_list_lumps.py [input.wad]
  If input.wad is omitted, uses data/doom2d.wad (or first .wad in current dir).
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from imports.wadfile import WadFile


def main() -> None:
    if len(sys.argv) >= 2:
        wad_path = sys.argv[1]
    else:
        default = "data/doom2d.wad"
        if os.path.isfile(default):
            wad_path = default
        else:
            for f in os.listdir("."):
                if f.lower().endswith(".wad"):
                    wad_path = f
                    break
            else:
                print("Usage: wad_list_lumps.py <input.wad>", file=sys.stderr)
                print("  or run from project root with data/doom2d.wad present.", file=sys.stderr)
                sys.exit(1)

    wad = WadFile.from_path(wad_path)
    lumps = wad.list_lumps()
    print(f"WAD: {wad_path}  id={wad.id.decode('ascii', errors='replace')}  lumps={len(lumps)}")
    print()
    for idx, name, size in lumps:
        print(f"{idx:5}  {size:8}  {name}")


if __name__ == "__main__":
    main()
