"""Generate the link glue of the PC executable (docs/port_runtime.md).

    python tools/port_link.py --objs <dir | file.rsp | file.obj> [...] --out <dir>

Reads the symbol tables of the compiled objects (game code + src/port/) and writes

    <out>/port_gen.obj    COFF object, linked into the exe
    <out>/port_link.txt   the link report: what was generated and why

port_gen.obj holds, for every symbol the objects use but do not define and that belongs to the
Wii executable (config/RM7P4F/symbols.txt, or an address comment in the sources):

  * a global          -> an absolute symbol at its original address (the runtime maps the Wii
                         address space there and loads the user's main.dol into it);
  * a function        -> a stub that jumps to the original address, where the runtime has put a
                         trap that reports the function's name and address and stops.

Both are weak (/alternatename): a real definition anywhere, also in the C runtime, wins, so a
stub disappears as soon as its function is decompiled.  The object also carries the tables the
runtime needs: every function of the executable (address, size, name, the PC implementation if
one is linked) and the byte-order layout of the initialised data.

Nothing here is game data: the tables are built from symbols.txt and the sources.
"""
import argparse, collections, glob, os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMS = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")
OVERRIDES = os.path.join(ROOT, "config", "RM7P4F", "port_data.txt")
SYMLINE = re.compile(r"^(\S+) = (\.?\w+):0x([0-9A-Fa-f]+); // type:(\w+)(?: size:0x([0-9A-Fa-f]+))?(.*)")
TEXT_LO, TEXT_HI = 0x80004000, 0x80345AE0          # .init + .text of main.dol
INIT_DATA = (".rodata", ".data", ".sdata", ".sdata2", ".ctors", ".dtors")

# data kinds of the layout table (include/port/port.h)
K_RAW, K_16, K_32, K_64, K_AUTO = 0, 2, 4, 8, 9
HINT_KIND = {"byte": K_RAW, "string": K_RAW, "string_table": K_RAW, "2byte": K_16, "wstring": K_16,
             "4byte": K_32, "float": K_32, "double": K_64}
# Declared types that say something about the layout. u8/char are left out on purpose: the
# decompiled code declares every untyped struct as a byte array.
TYPE_KIND = {"u16": K_16, "s16": K_16,
             "u32": K_32, "s32": K_32, "int": K_32, "f32": K_32, "float": K_32, "BOOL": K_32,
             "f64": K_64, "double": K_64}


# ---------------------------------------------------------------- COFF reading
def read_obj(path):
    """-> (defined external names, undefined external names) of one COFF object."""
    d = open(path, "rb").read()
    if len(d) < 20:
        return set(), set()
    machine, nsec, _, symoff, nsym = struct.unpack_from("<HHIII", d, 0)
    if machine == 0 and nsec == 0xFFFF:                 # /bigobj
        nsec, symoff, nsym = struct.unpack_from("<I", d, 44)[0], *struct.unpack_from("<II", d, 48)
        entry, fmt = 20, "<8sIiHBB"
    elif machine == 0x14C:
        entry, fmt = 18, "<8sIhHBB"
    else:
        sys.exit(f"{path}: not a 32-bit x86 COFF object (build for Win32, without /GL)")
    strtab = symoff + nsym * entry
    defined, undefined = set(), set()
    i = 0
    while i < nsym:
        raw, value, sec, _, cls, naux = struct.unpack_from(fmt, d, symoff + i * entry)
        i += 1 + naux
        if cls != 2:                                    # IMAGE_SYM_CLASS_EXTERNAL only
            continue
        if raw[:4] == b"\0\0\0\0":
            o = strtab + struct.unpack_from("<I", raw, 4)[0]
            name = d[o:d.index(b"\0", o)].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        if sec == 0 and value == 0:
            undefined.add(name)
        else:
            defined.add(name)                           # in a section, absolute, or common
    return defined, undefined


def base_name(sym):
    """Decorated linker name -> (source name, is a function or None if unknown)."""
    if sym.startswith("?"):
        m = re.match(r"\?+([^@?]+)@@(.)", sym)
        if not m:
            return None, None
        return m.group(1), m.group(2) in "YZ"           # Y = function, 3 = data
    if sym.startswith("__imp_") or sym.startswith("@"):
        return None, None
    return (sym[1:] if sym.startswith("_") else sym), None


def collect_objects(args):
    files = []
    for a in args:
        if os.path.isdir(a):
            files += glob.glob(os.path.join(a, "**", "*.obj"), recursive=True)
        elif a.lower().endswith(".rsp"):
            for l in re.split(r"[;\r\n]+", open(a).read()):
                l = l.strip().strip('"')
                if l:
                    files.append(l)
        else:
            files.append(a)
    return sorted(set(os.path.abspath(f) for f in files if os.path.basename(f).lower() != "port_gen.obj"))


# ---------------------------------------------------------------- symbols and sources
def load_symbols():
    """-> name -> (section, addr, type, size, data hint)."""
    out = {}
    for line in open(SYMS):
        m = SYMLINE.match(line)
        if m:
            name, sec, addr, typ, size, rest = m.groups()
            h = re.search(r"data:(\w+)", rest)
            out.setdefault(name, (sec, int(addr, 16), typ, int(size, 16) if size else 0, h.group(1) if h else None))
    return out


ADDR_NOTE = re.compile(r"(?://\s*|ORIG\()0x(8[0-9A-Fa-f]{7})\b")
MARKER = re.compile(r"^\s*//\s*0x(8[0-9A-Fa-f]{7})\s*$")
DEF_NAME = re.compile(r"([A-Za-z_]\w*)\s*\(")
EXTERN_DATA = re.compile(r"^\s*extern\s+(?:\"C\"\s+)?(?:const\s+|volatile\s+)*([A-Za-z_]\w*)\s*((?:\*\s*(?:const\s*)?)*)\s*([A-Za-z_]\w*)\s*(\[[^;]*)?;")
NOT_NAMES = {"if", "for", "while", "switch", "return", "sizeof", "static", "extern", "void", "int", "ORIG"}


PC_NO_INIT = {}   # addr -> name: globals the sources define without an initial value
MOVED = [0]
KEPT_INIT = []     # globals not moved: initialised in the sources, zero-filled in the original
ORIG_SECTION = lambda addr: None   # set in main(): original section name of an address
DEFS_BY_FILE = {}  # source file stem -> {name: original address} of the globals it defines with ORIG()


def relocate_globals(path):
    """Moves the globals a source file defines with ORIG(addr) to that address: their symbols in
    the object become absolute. The storage the compiler reserved stays behind unused; the value
    at the original address comes from main.dol (or is zero for .bss), as on the console. This
    way a global has one copy however it is reached: by its name, by `lbl_<addr>` from another
    file, or by an address inside it. -> number of symbols moved (0 if already done)."""
    defs = DEFS_BY_FILE.get(os.path.splitext(os.path.basename(path))[0].lower())
    if not defs:
        return 0
    d = bytearray(open(path, "rb").read())
    machine, nsec, _, symoff, nsym = struct.unpack_from("<HHIII", d, 0)
    if machine == 0 and nsec == 0xFFFF:                 # /bigobj
        symoff, nsym = struct.unpack_from("<II", d, 48)
        entry, fmt, absolute, sechdr = 20, "<8sIiHBB", -1, 56
    elif machine == 0x14C:
        entry, fmt, absolute, sechdr = 18, "<8sIhHBB", -1, 20 + struct.unpack_from("<H", d, 16)[0]
    else:
        return 0
    strtab = symoff + nsym * entry
    moved, i = 0, 0
    while i < nsym:
        at = symoff + i * entry
        raw, value, sec, typ, cls, naux = struct.unpack_from(fmt, d, at)
        i += 1 + naux
        if cls != 2 or sec <= 0 or naux:               # external definitions only: a static keeps its storage
            continue
        if raw[:4] == b"\0\0\0\0":
            o = strtab + struct.unpack_from("<I", raw, 4)[0]
            name = d[o:d.index(b"\0", o)].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        base, is_fn = base_name(name)
        if base in defs and not is_fn:
            # A value the sources give a global that is zero-filled in the original (.bss) would
            # be lost by the move: such a global keeps its storage in the executable.
            chars = struct.unpack_from("<I", d, sechdr + (sec - 1) * 40 + 36)[0]
            if not (chars & 0x80) and ORIG_SECTION(defs[base]) in (".bss", ".sbss", ".sbss2", None):
                KEPT_INIT.append(f"0x{defs[base]:08X} {base}")
                continue
            struct.pack_into(fmt, d, at, raw, defs[base], absolute, typ, cls, naux)
            moved += 1
    if moved:
        open(path, "wb").write(d)
    return moved


def scan_sources():
    """-> (notes: name -> Counter(addr) from address comments on declaration lines,
           markers: addr -> function name from '// 0xADDR' definition markers,
           decl_kind: name -> Counter(kind) from 'extern T name;' declarations,
           pc_defs: addr -> name for globals the sources define themselves (ORIG markers))."""
    notes = collections.defaultdict(collections.Counter)
    markers, decl_kind, pc_defs = {}, collections.defaultdict(collections.Counter), {}
    DEFS_BY_FILE.clear()
    PC_NO_INIT.clear()
    files = glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True) + \
        glob.glob(os.path.join(ROOT, "include", "**", "*.h"), recursive=True)
    for path in files:
        if os.sep + "port" + os.sep in path:
            continue
        lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
        for i, l in enumerate(lines):
            m = MARKER.match(l)
            if m:                                       # definition marker: the name is on a following line
                for nxt in lines[i + 1:i + 6]:
                    s = nxt.strip()
                    if not s or s.startswith("//"):
                        continue
                    cands = [n for n in DEF_NAME.findall(s.split("//")[0]) if n not in NOT_NAMES]
                    if cands:
                        markers.setdefault(int(m.group(1), 16), cands[0])
                    break
                continue
            a = ADDR_NOTE.search(l)
            code = l.split("//")[0]
            e = EXTERN_DATA.match(code)
            if e:
                typ, stars, name = e.group(1), e.group(2), e.group(3)
                kind = K_32 if stars.strip() else TYPE_KIND.get(typ)
                if kind is not None:
                    decl_kind[name][kind] += 1
            if not a:
                continue
            addr = int(a.group(1), 16)
            if "ORIG(" in l and "extern" not in code:   # a definition in the sources
                d = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(=[^;]*)?;\s*ORIG\(", l)
                if d:
                    pc_defs.setdefault(addr, d.group(1))
                    stem = os.path.splitext(os.path.basename(path))[0].lower()
                    DEFS_BY_FILE.setdefault(stem, {})[d.group(1)] = addr
                    if not d.group(2):
                        PC_NO_INIT[addr] = d.group(1)
                continue
            # declaration with an address note: the declared name is the identifier before ( [ ; =
            d = re.search(r"([A-Za-z_]\w*)\s*(?:\(|\[|;|=)", re.sub(r"\([^()]*\*\s*([A-Za-z_]\w*)\)\s*\(", r"\1(", code))
            if d and d.group(1) not in NOT_NAMES:
                notes[d.group(1)][addr] += 1
    return notes, markers, decl_kind, pc_defs


def load_overrides():
    """config/RM7P4F/port_data.txt: '<name|0xADDR> <layout>' where layout is a list of field
    widths (1 2 4 8, optionally NxCOUNT) repeated over the symbol."""
    out = {}
    if not os.path.exists(OVERRIDES):
        return out
    for line in open(OVERRIDES):
        t = line.split("#")[0].split()
        if len(t) < 2:
            continue
        widths = []
        for w in t[1:]:
            m = re.fullmatch(r"([1248])(?:x(\d+))?", w)
            if not m:
                sys.exit(f"port_data.txt: bad width '{w}' in: {line.strip()}")
            widths += [int(m.group(1))] * int(m.group(2) or 1)
        out[t[0]] = widths
    return out


# ---------------------------------------------------------------- COFF writing
class Coff:
    def __init__(self):
        self.strtab = b""
        self.syms = []          # (name, value, section number, storage class)
        self.index = {}

    def _name(self, n):
        b = n.encode("latin-1")
        if len(b) <= 8:
            return b.ljust(8, b"\0")
        off = 4 + len(self.strtab)
        self.strtab += b + b"\0"
        return struct.pack("<II", 0, off)

    def sym(self, name, value, sec, cls=2):
        if name in self.index:
            return self.index[name]
        self.index[name] = len(self.syms)
        self.syms.append((name, value, sec, cls))
        return self.index[name]

    def write(self, path, sections):
        """sections: [(name, data, characteristics, [(offset, symbol index, type)])]"""
        nsec = len(sections)
        off = 20 + 40 * nsec
        heads, body = b"", b""
        for name, data, chars, relocs in sections:
            if len(relocs) > 0xFFFF:
                sys.exit(f"port_link: {len(relocs)} relocations in {name}")
            rel = b"".join(struct.pack("<IIH", o, s, t) for o, s, t in relocs)
            heads += name.encode().ljust(8, b"\0") + struct.pack(
                "<IIIIIIHHI", 0, 0, len(data), off, off + len(data) if relocs else 0, 0, len(relocs), 0, chars)
            body += data + rel
            off += len(data) + len(rel)
        symtab = b"".join(self._name(n) + struct.pack("<IhHBB", v, s, 0, c, 0) for n, v, s, c in self.syms)
        hdr = struct.pack("<HHIIIHH", 0x14C, nsec, 0, off, len(self.syms), 0, 0)
        with open(path, "wb") as f:
            f.write(hdr + heads + body + symtab + struct.pack("<I", 4 + len(self.strtab)) + self.strtab)


# ---------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--objs", action="append", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()

    objs = collect_objects(a.objs)
    if not objs:
        sys.exit("port_link: no object files")
    syms = load_symbols()
    notes, markers, decl_kind, pc_defs = scan_sources()
    global ORIG_SECTION
    by_start = sorted((v[1], v[0]) for v in syms.values() if v[2] in ("object", "label"))
    starts = [x[0] for x in by_start]

    def orig_section(addr):
        import bisect
        i = bisect.bisect_right(starts, addr) - 1
        return by_start[i][1] if i >= 0 else None
    ORIG_SECTION = orig_section
    MOVED[0] = sum(relocate_globals(o) for o in objs)
    defined, undefined = set(), set()
    for o in objs:
        d, u = read_obj(o)
        defined |= d
        undefined |= u
    undefined -= defined

    by_addr_fn = {}
    for name, (sec, addr, typ, size, hint) in syms.items():
        if typ == "function":
            by_addr_fn.setdefault(addr, (name, size))
    overrides = load_overrides()

    coff = Coff()
    directives, text, report = [], bytearray(), collections.defaultdict(list)
    SEC_TEXT, SEC_RDATA = 2, 3                           # 1 = .drectve

    def address_of(name):
        """-> (addr, is function, size, where from) or None."""
        s = syms.get(name)
        if s and s[2] in ("function", "object", "label"):
            return s[1], s[2] == "function" or (s[2] == "label" and TEXT_LO <= s[1] < TEXT_HI), s[3], "symbols.txt"
        m = re.search(r"(?:^(?:lbl|fn|jumptable)_|[A-Za-z])(8[0-9A-F]{7})$", name)
        if m and name not in notes:                      # the name carries the address (lbl_803C73EC, gBuffer803C73EC)
            addr = int(m.group(1), 16)
            return addr, TEXT_LO <= addr < TEXT_HI, 0, "name"
        if name in notes:
            ranked = notes[name].most_common()
            if len(ranked) > 1:
                report["conflict"].append(f"{name}: " + ", ".join(f"0x{x:08X} x{n}" for x, n in ranked))
            addr = ranked[0][0]
            return addr, TEXT_LO <= addr < TEXT_HI, 0, "source comment"
        return None

    # ---- PC implementations: decompiled functions the objects define, by original address
    impl_of = {}                                         # addr -> defined symbol
    by_base = collections.defaultdict(list)
    for sym in defined:
        n, _ = base_name(sym)
        if n:
            by_base[n].append(sym)
    static_fns = []
    for addr, name in markers.items():
        by_addr_fn.setdefault(addr, (name, 0))
    for addr, (name, size) in by_addr_fn.items():
        for cand in (markers.get(addr), name, f"fn_{addr:08X}"):   # also a definition still under its fn_ name after symbols.txt renamed it
            if cand and cand in by_base and len(by_base[cand]) == 1:
                s = by_base[cand][0]
                if not s.startswith("?") or base_name(s)[1]:
                    impl_of[addr] = s
                    break
        else:
            if addr in markers:
                static_fns.append((addr, markers[addr]))
    stubs = {}                                           # addr -> stub symbol
    for sym in sorted(undefined):
        name, is_fn = base_name(sym)
        if name is None:
            continue
        got = address_of(name)
        if not got:
            report["system"].append(sym)
            continue
        addr, fn_by_addr, size, src = got
        if is_fn is None:
            is_fn = fn_by_addr
        if is_fn != fn_by_addr:
            report["conflict"].append(f"{sym}: used as {'function' if is_fn else 'data'} but 0x{addr:08X} is {'code' if fn_by_addr else 'data'}")
        if fn_by_addr and addr in impl_of:               # decompiled under another name
            target = impl_of[addr]
            report["alias"].append(f"{name} -> {target} (0x{addr:08X})")
        elif not fn_by_addr and addr in pc_defs and len(by_base.get(pc_defs[addr], [])) == 1                 and base_name(by_base[pc_defs[addr]][0])[1] is not True:
            target = by_base[pc_defs[addr]][0]           # the sources define this global: use that copy
            report["alias"].append(f"{name} -> {target} (0x{addr:08X})")
        elif fn_by_addr:
            target = stubs.get(addr)
            if target is None:
                target = stubs[addr] = f"stub${addr:08X}"
                coff.sym(target, len(text), SEC_TEXT)
                text += b"\xB8" + struct.pack("<I", addr) + b"\xFF\xE0\xCC"     # mov eax, addr; jmp eax
            report["function"].append((addr, name, size, src))
        else:
            target = f"abs${addr:08X}"
            coff.sym(target, addr, -1)
            report["global"].append((addr, name, src))
            if addr in pc_defs:
                report["split"].append(f"0x{addr:08X}: defined in the sources as {pc_defs[addr]}, used at the original address as {name}")
        directives.append(f"/alternatename:{sym}={target}")

    # ---- function table: every function of the executable, with its PC implementation if linked
    names, name_off = bytearray(b"\0"), {}
    rdata, relocs = bytearray(), []
    funcs = sorted(by_addr_fn.items())
    for addr, (name, size) in funcs:
        shown = markers.get(addr, name)
        if shown not in name_off:
            name_off[shown] = len(names)
            names += shown.encode() + b"\0"
        if addr in impl_of:
            relocs.append((len(rdata) + 8, coff.sym(impl_of[addr], 0, 0), 6))      # IMAGE_REL_I386_DIR32
        rdata += struct.pack("<IIII", addr, size, 0, name_off[shown])
    func_count = len(funcs)

    # ---- data layout table: how to byte-swap the initialised data of main.dol
    layout = []                                          # (addr, size, kind)
    kinds = collections.Counter()
    data_syms = sorted((s[1], n, s) for n, s in syms.items() if s[2] == "object" and s[0] in INIT_DATA and s[3])
    for addr, name, (sec, _, _, size, hint) in data_syms:
        ov = overrides.get(name) or overrides.get(f"0x{addr:08X}")
        if ov:
            pos = 0
            while pos < size:
                for w in ov:
                    if pos + w > size:
                        pos = size
                        break
                    layout.append((addr + pos, w, K_RAW if w == 1 else w))
                    pos += w
            kinds["override"] += 1
            continue
        decl = decl_kind.get(name)
        if hint in HINT_KIND:
            kind, why = HINT_KIND[hint], "hint"
        elif decl and len(decl) == 1:
            kind, why = next(iter(decl)), "declared type"
        elif sec in (".ctors", ".dtors") or name.startswith("jumptable_"):
            kind, why = K_32, "pointer table"
        else:
            kind, why = K_AUTO, "auto"
        if kind in (K_16, K_32, K_64) and (size % kind or addr % min(kind, 4)):
            kind, why = K_AUTO, "auto"
        kinds[why] += 1
        layout.append((addr, size, kind))
    merged = []
    for addr, size, kind in layout:
        if merged and merged[-1][2] == kind and kind != K_AUTO and merged[-1][0] + merged[-1][1] == addr:
            merged[-1] = (merged[-1][0], merged[-1][1] + size, kind)
        else:
            merged.append((addr, size, kind))
    funcs_off = 0
    data_off = len(rdata)
    for addr, size, kind in merged:
        rdata += struct.pack("<III", addr, size, kind)
    names_off = len(rdata)
    rdata += names
    while len(rdata) % 4:
        rdata += b"\0"
    counts_off = len(rdata)
    rdata += struct.pack("<II", func_count, len(merged))

    coff.sym("_gPortFuncs", funcs_off, SEC_RDATA)
    coff.sym("_gPortData", data_off, SEC_RDATA)
    coff.sym("_gPortNames", names_off, SEC_RDATA)
    coff.sym("_gPortFuncCount", counts_off, SEC_RDATA)
    coff.sym("_gPortDataCount", counts_off + 4, SEC_RDATA)

    coff.sym("@feat.00", 1, -1, 3)                       # safe-SEH compatible (no handlers)
    os.makedirs(a.out, exist_ok=True)
    drectve = (" ".join(directives) + " ").encode("latin-1")
    coff.write(os.path.join(a.out, "port_gen.obj"), [
        (".drectve", drectve, 0x00100A00, []),                     # LNK_INFO | LNK_REMOVE | ALIGN_1
        (".text", bytes(text) or b"\xCC", 0x60500020, []),         # CODE | EXECUTE | READ | ALIGN_16
        (".rdata", bytes(rdata), 0x40300040, relocs),              # INITIALIZED_DATA | READ | ALIGN_4
    ])

    fns = sorted(set(report["function"]))
    game = [f for f in fns if 0x80006B40 <= f[0] < 0x8022FEC4]
    lib = [f for f in fns if f not in game]
    with open(os.path.join(a.out, "port_link.txt"), "w", newline="\n") as f:
        f.write(f"objects: {len(objs)}\n")
        f.write(f"functions of the executable: {func_count}, linked PC implementations: {len(impl_of)}\n")
        f.write(f"stubs: {len(game)} game functions, {len(lib)} library functions\n")
        f.write(f"globals at their original address: {len(set(x[0] for x in report['global']))} ({len(report['global'])} names)\n")
        f.write(f"data layout: {len(merged)} runs; " + ", ".join(f"{k} {v}" for k, v in kinds.most_common()) + "\n")
        for title, key in (("Address conflicts (fix in the sources)", "conflict"),
                           ("Names resolved to a definition under another name", "alias"),
                           ("Split globals: defined in the sources AND used at the original address (two copies)", "split")):
            f.write(f"\n{title}: {len(report[key])}\n" + "".join(f"  {x}\n" for x in sorted(set(report[key]))))
        # A global the sources define does not get its bytes from main.dol. If the original keeps
        # it in an initialised section, a definition without a value starts as 0 instead.
        sec_of = {v[1]: v[0] for v in syms.values()}
        noinit = sorted((x, n, sec_of[x]) for x, n in PC_NO_INIT.items()
                        if sec_of.get(x) in (".data", ".sdata", ".sdata2", ".rodata"))
        f.write(f"\nDefined in the sources without an initial value, but in an initialised section of the "
                f"original (starts as 0 unless main.dol has 0 there too): {len(noinit)}\n")
        f.write("".join(f"  0x{x:08X} {n} ({sec})\n" for x, n, sec in noinit))
        f.write(f"\nDecompiled as static (not reachable through their original address): {len(static_fns)}\n")
        f.write("".join(f"  0x{x:08X} {n}\n" for x, n in sorted(static_fns)))
        f.write(f"\nGame function stubs: {len(game)}\n" + "".join(f"  0x{x:08X} {n} size 0x{s:X} ({src})\n" for x, n, s, src in game))
        f.write(f"\nLibrary function stubs: {len(lib)}\n" + "".join(f"  0x{x:08X} {n} ({src})\n" for x, n, s, src in lib))
        f.write(f"\nGlobals the sources define, moved to their original address: {MOVED[0]} in this run "
                f"(objects already processed are not counted)\n")
        kept = sorted(set(KEPT_INIT))
        f.write(f"\nNot moved, because the sources give them a value and the original has them in .bss "
                f"(they keep a copy of their own in the executable): {len(kept)}\n")
        f.write("".join(f"  {x}\n" for x in kept))
        f.write(f"\nLeft to the system libraries (C runtime, Windows): {len(report['system'])}\n")
        f.write("".join(f"  {x}\n" for x in report["system"]))
    print(f"port_link: {len(objs)} objects, {len(impl_of)}/{func_count} functions linked, "
          f"{len(game)} game + {len(lib)} library stubs, {len(report['global'])} globals, "
          f"{len(set(report['split']))} split globals, {len(set(report['conflict']))} conflicts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
