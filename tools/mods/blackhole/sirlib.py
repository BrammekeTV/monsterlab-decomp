"""Minimal reader for the game's .sir containers (PNP0), for the black hole mod tools.

Reads from the user's own extracted disc; nothing read here is ever written into the repository."""
import os
import struct


def data_dir():
    d = os.environ.get("MONSTERLAB_DATA")
    if not d:
        root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
        d = os.path.join(root, "orig", "RM7P4F")
    return d


def stream(name):
    return os.path.join(data_dir(), "files", "streams", name)


class Sir:
    def __init__(self, path):
        self.path = path
        self.f = open(path, "rb")
        head = self.f.read(0x40)
        if head[:4] != b"PNP0":
            raise ValueError(path + ": not a PNP0 file")
        self.count = struct.unpack_from(">I", head, 8)[0]
        tab = self.f.read(24 * self.count)
        # (type, offset, size, w3, w4, hash)
        self.sections = [struct.unpack_from(">iIIIII", tab, 24 * i) for i in range(self.count)]

    def read(self, i):
        _, off, size = self.sections[i][:3]
        self.f.seek(off)
        return self.f.read(size)

    def find_type(self, t):
        for i, s in enumerate(self.sections):
            if s[0] == t:
                return i
        return -1


def be32(b, o):
    return struct.unpack_from(">I", b, o)[0]


def string_hash(s):
    """The game's StringHash: CRC-32 of the upper-cased name."""
    import zlib
    return zlib.crc32(s.upper().encode("latin1")) & 0xFFFFFFFF
