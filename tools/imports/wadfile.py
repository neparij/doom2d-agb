"""
WAD file reader and writer (Doom / Doom2D format).

Format:
- Header 12 bytes: id[4] ("IWAD"/"PWAD"), numlumps LE32, directory_offset LE32
- Directory: numlumps entries, 16 bytes each: filepos LE32, size LE32, name[8]
All multi-byte integers are little-endian.
Lump names may be in CP866 (DOS Cyrillic) or ASCII.
"""

import struct
from typing import BinaryIO, List, Optional, Tuple, Union

# Кодировка имён лумпов (Doom2D и др. могут использовать кириллицу в cp866)
LUMP_NAME_ENCODING = "cp866"


class WadFile:
    """Read-only view of a WAD file (in-memory or file-backed)."""

    DIR_ENTRY_SIZE = 16
    HEADER_SIZE = 12
    LUMP_NAME_SIZE = 8

    def __init__(self, data: bytes):
        self._data = data
        if len(data) < self.HEADER_SIZE:
            raise ValueError("WAD too small for header")
        self._id = data[0:4]
        self._num_lumps = struct.unpack("<I", data[4:8])[0]
        self._dir_offset = struct.unpack("<I", data[8:12])[0]
        required = self._dir_offset + self._num_lumps * self.DIR_ENTRY_SIZE
        if len(data) < required:
            raise ValueError(
                f"WAD truncated: need {required} bytes for directory, have {len(data)}"
            )

    @classmethod
    def from_path(cls, path: str) -> "WadFile":
        """Load WAD from file path."""
        with open(path, "rb") as f:
            return cls(f.read())

    def write(self, path: str) -> None:
        """Write WAD to file path (full overwrite)."""
        with open(path, "wb") as f:
            f.write(self._data)

    @property
    def id(self) -> bytes:
        """WAD type: b'IWAD' or b'PWAD'."""
        return self._id

    @property
    def num_lumps(self) -> int:
        return self._num_lumps

    def _entry_offset(self, index: int) -> int:
        return self._dir_offset + index * self.DIR_ENTRY_SIZE

    def get_lump_info(
        self, index: int
    ) -> Tuple[int, int, bytes]:
        """
        Return (filepos, size, name_bytes) for lump at index.
        name_bytes is exactly 8 bytes (may be null/space padded).
        """
        if index < 0 or index >= self._num_lumps:
            raise IndexError(f"Lump index {index} out of range [0, {self._num_lumps})")
        ofs = self._entry_offset(index)
        entry = self._data[ofs : ofs + self.DIR_ENTRY_SIZE]
        filepos, size = struct.unpack("<II", entry[0:8])
        name = entry[8:16]
        return (filepos, size, name)

    @classmethod
    def decode_lump_name(cls, name_bytes: bytes) -> str:
        """Decode 8-byte WAD lump name to string (CP866/ASCII)."""
        raw = name_bytes.split(b"\x00")[0].rstrip(b" ")
        return raw.decode(LUMP_NAME_ENCODING, errors="replace")

    def get_lump_name(self, index: int) -> str:
        """Lump name as string (strip null/space padding, CP866)."""
        _, _, name_bytes = self.get_lump_info(index)
        return self.decode_lump_name(name_bytes)

    def get_lump_data(self, index: int) -> bytes:
        """Raw bytes of the lump."""
        filepos, size, _ = self.get_lump_info(index)
        end = filepos + size
        if end > len(self._data):
            raise ValueError(f"Lump {index} extends past end of WAD")
        return self._data[filepos:end]

    def _name_matches(self, wad_name: bytes, want: str) -> bool:
        """Compare 8-byte WAD name with string (case-insensitive, CP866 decode)."""
        decoded = self.decode_lump_name(wad_name)
        return decoded.lower() == want.lower()

    def find_lump(self, name: str) -> Optional[Tuple[int, bytes]]:
        """
        Find first lump with given name (case-insensitive, 8-char padded).
        Returns (lump_index, data) or None if not found.
        """
        for i in range(self._num_lumps):
            _, _, name_bytes = self.get_lump_info(i)
            if self._name_matches(name_bytes, name):
                return (i, self.get_lump_data(i))
        return None

    def lumps_by_prefix(self, prefix: str) -> List[Tuple[int, str, bytes]]:
        """
        All lumps whose name starts with prefix (case-insensitive).
        Returns list of (index, name_str, data).
        """
        prefix_upper = prefix.upper()
        result = []
        for i in range(self._num_lumps):
            _, _, name_bytes = self.get_lump_info(i)
            name_str = self.decode_lump_name(name_bytes)
            if name_str.upper().startswith(prefix_upper):
                result.append((i, name_str, self.get_lump_data(i)))
        return result

    def list_lumps(self) -> List[Tuple[int, str, int]]:
        """All lumps as (index, name, size). Names decoded with CP866."""
        out = []
        for i in range(self._num_lumps):
            filepos, size, name_bytes = self.get_lump_info(i)
            name_str = self.decode_lump_name(name_bytes)
            out.append((i, name_str, size))
        return out

    @classmethod
    def _pack_name(cls, name: str) -> bytes:
        """Encode lump name to 8 bytes (CP866, null padded)."""
        raw = name.encode(LUMP_NAME_ENCODING, errors="replace")[: cls.LUMP_NAME_SIZE]
        return raw.ljust(cls.LUMP_NAME_SIZE, b"\x00")

    def to_lumps(self) -> List[Tuple[bytes, bytes]]:
        """All lumps as list of (name_8bytes, data). Preserves order and exact names."""
        out = []
        for i in range(self._num_lumps):
            _, _, name_bytes = self.get_lump_info(i)
            out.append((name_bytes, self.get_lump_data(i)))
        return out

    @classmethod
    def from_lumps(cls, wad_id: bytes, lumps: List[Tuple[bytes, bytes]]) -> "WadFile":
        """
        Build a valid WAD from list of (name_8bytes, data). name_8bytes must be 8 bytes each.
        Returns a new WadFile with correct header and directory.
        """
        if len(wad_id) != 4:
            raise ValueError("wad_id must be 4 bytes (e.g. b'IWAD' or b'PWAD')")
        data_parts = []
        dir_entries = []
        filepos = cls.HEADER_SIZE
        for name_8, payload in lumps:
            if len(name_8) != cls.LUMP_NAME_SIZE:
                raise ValueError(f"Lump name must be {cls.LUMP_NAME_SIZE} bytes, got {len(name_8)}")
            data_parts.append(payload)
            dir_entries.append((filepos, len(payload), name_8))
            filepos += len(payload)
        dir_offset = filepos
        num_lumps = len(lumps)
        # Build: header + lump data + directory
        out = bytearray()
        out += wad_id
        out += struct.pack("<I", num_lumps)
        out += struct.pack("<I", dir_offset)
        for payload in data_parts:
            out += payload
        for (fp, sz, name_8) in dir_entries:
            out += struct.pack("<II", fp, sz)
            out += name_8
        return cls(bytes(out))

    def remove_lump(
        self,
        *,
        name: Optional[str] = None,
        index: Optional[int] = None,
    ) -> "WadFile":
        """
        Return a new WAD with one lump removed. Specify either name or index.
        """
        lumps = self.to_lumps()
        if index is not None:
            if index < 0 or index >= len(lumps):
                raise IndexError(f"Lump index {index} out of range [0, {len(lumps)})")
            lumps.pop(index)
        elif name is not None:
            for i in range(len(lumps)):
                if self._name_matches(lumps[i][0], name):
                    lumps.pop(i)
                    break
            else:
                raise LookupError(f"Lump named {name!r} not found")
        else:
            raise ValueError("Specify either name= or index=")
        return self.from_lumps(self._id, lumps)

    def add_lump(self, name: str, data: bytes, index: Optional[int] = None) -> "WadFile":
        """
        Return a new WAD with one lump added. index=None appends at end.
        """
        lumps = self.to_lumps()
        name_8 = self._pack_name(name)
        entry = (name_8, data)
        if index is None:
            lumps.append(entry)
        else:
            if index < 0 or index > len(lumps):
                raise IndexError(f"Lump index {index} out of range [0, {len(lumps)}]")
            lumps.insert(index, entry)
        return self.from_lumps(self._id, lumps)

    def replace_lump(
        self,
        data: bytes,
        *,
        name: Optional[str] = None,
        index: Optional[int] = None,
    ) -> "WadFile":
        """
        Return a new WAD with one lump's data replaced. Specify either name or index.
        """
        lumps = self.to_lumps()
        if index is not None:
            if index < 0 or index >= len(lumps):
                raise IndexError(f"Lump index {index} out of range [0, {len(lumps)})")
            lumps[index] = (lumps[index][0], data)
        elif name is not None:
            want = self._pack_name(name)
            for i in range(len(lumps)):
                if lumps[i][0] == want or self._name_matches(lumps[i][0], name):
                    lumps[i] = (lumps[i][0], data)
                    break
            else:
                raise LookupError(f"Lump named {name!r} not found")
        else:
            raise ValueError("Specify either name= or index=")
        return self.from_lumps(self._id, lumps)
