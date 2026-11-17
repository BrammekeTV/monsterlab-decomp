"""Cross-reference database built from dtk's asm output + the original DOL.

Usage (as a library):
    from xref import DB
    db = DB()                       # loads everything
    db.funcs['main']                # Func(name, addr, size, file, calls, refs)
    db.strings_of('main')           # C strings referenced by a function
    db.callers['OSReport']          # who calls a function

CLI:
    python tools/xref.py func <name|addr>      # show calls / strings / callers
    python tools/xref.py strings <regex>       # functions referencing matching strings
"""
import os
import re
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass, field

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOL = os.path.join(ROOT, "orig", "RM7P4F", "sys", "main.dol")
ASM = os.path.join(ROOT, "build", "RM7P4F", "asm")
SYMS = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")

SYM_RE = re.compile(r"^(\S+) = (\.[\w]+|\w+):0x([0-9A-Fa-f]+); // (.*)$")
FN_RE = re.compile(r"^\.fn (\S+), (\w+)")
INS_RE = re.compile(r"^/\* ([0-9A-F]{8}) [0-9A-F]{8}  [0-9A-F ]{11} \*/\t(\S+)\s*(.*)$")
REF_RE = re.compile(r"\b([A-Za-z_.$?@][\w.$?@]*)(?:@ha|@l|@h|@sda21|@sda2)\b")
BRANCH_OPS = {"bl", "b"}


@dataclass
class Func:
    name: str
    addr: int
    size: int
    file: str
    calls: list = field(default_factory=list)  # ordered, may repeat
    tail: list = field(default_factory=list)   # tail calls (b to another fn)
    refs: list = field(default_factory=list)   # data symbol refs


class Dol:
    def __init__(self, path=DOL):
        d = open(path, "rb").read()
        offs = struct.unpack(">18I", d[0:72])
        addrs = struct.unpack(">18I", d[72:144])
        sizes = struct.unpack(">18I", d[144:216])
        self.sections = [(a, s, d[o:o + s]) for o, a, s in zip(offs, addrs, sizes) if s]

    def read(self, addr, n):
        for a, s, b in self.sections:
            if a <= addr < a + s:
                return b[addr - a:addr - a + n]
        return None

    def cstring(self, addr, maxlen=256):
        b = self.read(addr, maxlen)
        if not b:
            return None
        end = b.find(b"\0")
        if end <= 0:
            return None
        raw = b[:end]
        if all(32 <= c < 127 or c in (9, 10, 13) for c in raw):
            return raw.decode("ascii")
        return None

    def u32(self, addr):
        b = self.read(addr, 4)
        return struct.unpack(">I", b)[0] if b and len(b) == 4 else None


class DB:
    def __init__(self):
        self.dol = Dol()
        self.syms = {}      # name -> (section, addr, meta)
        self.by_addr = {}
        for line in open(SYMS):
            m = SYM_RE.match(line.strip())
            if m:
                name, sec, addr, meta = m.group(1), m.group(2), int(m.group(3), 16), m.group(4)
                self.syms[name] = (sec, addr, meta)
                self.by_addr.setdefault(addr, name)
        self.funcs = {}
        self.callers = defaultdict(set)
        self._parse_asm()

    def _parse_asm(self):
        for fn in sorted(os.listdir(ASM)):
            if not fn.endswith(".s"):
                continue
            cur = None
            for line in open(os.path.join(ASM, fn)):
                if line.startswith(".fn "):
                    name = FN_RE.match(line).group(1)
                    sec, addr, meta = self.syms.get(name, (None, 0, ""))
                    size = int(re.search(r"size:0x([0-9A-F]+)", meta).group(1), 16) if "size:" in meta else 0
                    cur = Func(name, addr, size, fn)
                    self.funcs[name] = cur
                elif line.startswith(".endfn"):
                    cur = None
                elif cur is not None:
                    m = INS_RE.match(line)
                    if not m:
                        continue
                    op, args = m.group(2), m.group(3)
                    if op in BRANCH_OPS and args and not args.startswith(".L"):
                        tgt = args.split()[0]
                        (cur.calls if op == "bl" else cur.tail).append(tgt)
                        self.callers[tgt].add(cur.name)
                    for r in REF_RE.findall(args):
                        if r not in cur.refs:
                            cur.refs.append(r)
                    # function pointers loaded via lis/addi show up as refs too

    def addr_of(self, name):
        return self.syms[name][1] if name in self.syms else None

    def strings_of(self, fname):
        out = []
        for r in self.funcs[fname].refs:
            a = self.addr_of(r)
            if a is None:
                continue
            s = self.dol.cstring(a)
            if s is not None:
                out.append(s)
        return out

    def resolve(self, key):
        if key in self.funcs:
            return self.funcs[key]
        try:
            a = int(key, 16)
        except ValueError:
            return None
        for f in self.funcs.values():
            if f.addr <= a < f.addr + max(f.size, 4):
                return f
        return None


def main():
    db = DB()
    if len(sys.argv) < 3:
        print(__doc__)
        return
    cmd, arg = sys.argv[1], sys.argv[2]
    if cmd == "func":
        f = db.resolve(arg)
        if not f:
            print("not found")
            return
        print(f"{f.name} @ {f.addr:08X} size 0x{f.size:X} ({f.file})")
        print("calls:  ", ", ".join(dict.fromkeys(f.calls)))
        if f.tail:
            print("tail:   ", ", ".join(dict.fromkeys(f.tail)))
        print("refs:   ", ", ".join(r for r in f.refs if not r.startswith("_SDA")))
        for s in db.strings_of(f.name):
            print("  str:", repr(s))
        print("callers:", ", ".join(sorted(db.callers.get(f.name, ()))))
    elif cmd == "strings":
        pat = re.compile(arg)
        for f in sorted(db.funcs.values(), key=lambda f: f.addr):
            hits = [s for s in db.strings_of(f.name) if pat.search(s)]
            if hits:
                print(f"{f.addr:08X} {f.name}: " + " | ".join(repr(h) for h in hits[:4]))


if __name__ == "__main__":
    main()
