"""Name library functions in main.dol by matching them against compiled public SDK source.

The object files come from compiling a public matching decomp's SDK/MSL/MetroTRK source with
the Metrowerks compiler (kept outside the repo, never committed). Each object function is
reduced to its words with relocated fields masked (branch targets, @ha/@l/@sda21 operands);
a main.dol function matches when its size is equal and every unmasked bit is equal.
Only unique matches are written.

Usage:
    python tools/sig_match.py --objs DIR [--dol main.dol] [--min-size 24] [--out FILE]

Output lines are `fn_XXXXXXXX name` (same format as config/RM7P4F/names_*.txt), followed by
`# conflict:` comments for matches that were rejected.
"""
import argparse
import os
import re
import struct
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMS = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")
NAME_FILES = ["names_sdk.txt", "names_game.txt", "names_script.txt", "names_strings.txt"]

R_PPC_ADDR16_LO, R_PPC_ADDR16_HI, R_PPC_ADDR16_HA = 4, 5, 6
R_PPC_REL24, R_PPC_REL14, R_PPC_ADDR16 = 10, 11, 3
R_PPC_EMB_SDA21 = 109
R_MASK = {
    R_PPC_REL24: 0x03FFFFFC,
    R_PPC_REL14: 0x0000FFFC,
    R_PPC_ADDR16: 0xFFFF,
    R_PPC_ADDR16_LO: 0xFFFF,
    R_PPC_ADDR16_HI: 0xFFFF,
    R_PPC_ADDR16_HA: 0xFFFF,
    R_PPC_EMB_SDA21: 0x001FFFFF,
}


def read_obj(path):
    """Yield (name, bytes, masks, calls) for every function symbol in a PPC ELF object."""
    d = open(path, "rb").read()
    if d[:4] != b"\x7fELF":
        return
    shoff = struct.unpack(">I", d[32:36])[0]
    shentsize, shnum, shstr = struct.unpack(">HHH", d[46:52])
    sh = [struct.unpack(">10I", d[shoff + i * shentsize: shoff + i * shentsize + 40]) for i in range(shnum)]

    def secname(s):
        return d[sh[shstr][4] + s[0]:].split(b"\0")[0].decode()

    names = [secname(s) for s in sh]
    symtab = next(i for i, s in enumerate(sh) if s[1] == 2)
    strtab = sh[sh[symtab][6]]
    syms = []
    binds = {}
    for off in range(sh[symtab][4], sh[symtab][4] + sh[symtab][5], 16):
        n, v, sz, info, _o, shndx = struct.unpack(">IIIBBH", d[off:off + 16])
        nm = d[strtab[4] + n:].split(b"\0")[0].decode()
        syms.append((nm, v, sz, info & 15, shndx))
        binds[nm] = info >> 4
    for ti, tn in enumerate(names):
        if tn not in (".text", ".init"):
            continue
        text = d[sh[ti][4]: sh[ti][4] + sh[ti][5]]
        mask = [0] * (len(text) // 4)
        rel = {}
        for ri, s in enumerate(sh):
            if s[1] == 4 and s[7] == ti:
                for off in range(s[4], s[4] + s[5], 12):
                    o, info, _a = struct.unpack(">IIi", d[off:off + 12])
                    t, si = info & 0xFF, info >> 8
                    if t in R_MASK and o // 4 < len(mask):
                        mask[o // 4] = R_MASK[t]
                        rel[o] = (t, syms[si][0])
        for nm, v, sz, ty, shndx in syms:
            if ty == 2 and shndx == ti and sz >= 4 and not nm.startswith("@"):
                lo, hi = v // 4, (v + sz) // 4
                calls = {o - v: r for o, r in rel.items() if v <= o < v + sz}
                yield nm, text[v:v + sz], mask[lo:hi], calls, binds[nm]


def read_dol(path):
    d = open(path, "rb").read()
    offs = struct.unpack(">18I", d[0:72])
    addrs = struct.unpack(">18I", d[72:144])
    sizes = struct.unpack(">18I", d[144:216])
    return [(a, d[o:o + s]) for o, a, s in zip(offs, addrs, sizes) if s and a]


def dol_bytes(secs, addr, size):
    for a, b in secs:
        if a <= addr and addr + size <= a + len(b):
            return b[addr - a: addr - a + size]
    return None


def load_symbols():
    funcs, byaddr = [], {}
    pat = re.compile(r"^(\S+) = \.\w+:0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)")
    for line in open(SYMS):
        m = pat.match(line)
        if m:
            nm, a, sz = m.group(1), int(m.group(2), 16), int(m.group(3), 16)
            byaddr[a] = nm
            funcs.append((nm, a, sz))
    return funcs, byaddr


def known_names():
    out = set()
    for f in NAME_FILES:
        p = os.path.join(ROOT, "config", "RM7P4F", f)
        if os.path.exists(p):
            for line in open(p):
                parts = line.split("#")[0].split()
                if len(parts) == 2:
                    out.add(parts[1])
    return out


def bl_target(word, addr):
    if word >> 26 != 18:
        return None
    off = word & 0x03FFFFFC
    if off & 0x02000000:
        off -= 0x04000000
    if word & 2:
        return off & 0xFFFFFFFF
    return (addr + off) & 0xFFFFFFFF


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--objs", required=True)
    ap.add_argument("--dol", default=os.path.join(ROOT, "orig", "RM7P4F", "sys", "main.dol"))
    ap.add_argument("--min-size", type=int, default=24)
    ap.add_argument("--min-addr", type=lambda x: int(x, 0), default=0x80230000)
    ap.add_argument("--out")
    a = ap.parse_args()
    secs = read_dol(a.dol)
    funcs, byaddr = load_symbols()
    used = known_names()

    # DOL candidates: unnamed fn_ functions in the library range, bucketed by size
    cand = defaultdict(list)
    for nm, addr, sz in funcs:
        if nm.startswith("fn_") and addr >= a.min_addr and sz >= a.min_size:
            b = dol_bytes(secs, addr, sz)
            if b:
                cand[sz].append((addr, nm, struct.unpack(">%dI" % (sz // 4), b)))

    objfuncs = []
    for root, _d, files in os.walk(a.objs):
        for f in files:
            if f.endswith(".o"):
                for nm, b, mask, calls, bind in read_obj(os.path.join(root, f)):
                    if len(b) >= a.min_size and len(b) % 4 == 0:
                        objfuncs.append((nm, os.path.relpath(os.path.join(root, f), a.objs).replace(os.sep, '/'), struct.unpack(">%dI" % (len(b) // 4), b), mask, calls, bind))

    by_obj = defaultdict(set)   # (objname) -> dol addrs
    by_dol = defaultdict(set)   # dol addr -> objnames
    for nm, f, w, mask, calls, bind in objfuncs:
        for addr, _dn, dw in cand.get(len(w) * 4, ()):
            ok = True
            for i, (x, y, m) in enumerate(zip(w, dw, mask)):
                if (x ^ y) & ~m & 0xFFFFFFFF:
                    # an unrelocated branch outside the function is still an external call
                    ok = False
                    break
            if ok:
                by_obj[nm].add(addr)
                by_dol[addr].add(nm)

    result, conflicts = {}, []
    for addr, nms in sorted(by_dol.items()):
        if len(nms) != 1:
            conflicts.append((addr, "ambiguous: " + ",".join(sorted(nms))))
            continue
        nm = next(iter(nms))
        if len(by_obj[nm]) != 1:
            conflicts.append((addr, "%s also matches %d other functions" % (nm, len(by_obj[nm]) - 1)))
            continue
        if nm in used or nm in byaddr.values():
            conflicts.append((addr, "%s already used" % nm))
            continue
        result[addr] = nm

    # a static (local) function's name is only usable when no other object function has it
    cnt = defaultdict(int)
    for o in objfuncs:
        cnt[o[0]] += 1
    objfile = {o[0]: o[1] for o in objfuncs}
    objbind = {o[0]: o[5] for o in objfuncs}
    for addr, nm in list(result.items()):
        if objbind[nm] == 0 and cnt[nm] > 1:
            conflicts.append((addr, '%s: static name defined %d times' % (nm, cnt[nm])))
            del result[addr]

    # call-target consistency: a callee already named differently rejects the match
    objby = {}
    for nm, f, w, mask, calls, bind in objfuncs:
        objby.setdefault(nm, (w, calls))
    final = {}
    for addr, nm in result.items():
        w, calls = objby[nm]
        bad = None
        for off, (t, sym) in calls.items():
            if t != R_PPC_REL24:
                continue
            tgt = bl_target(w[off // 4] if False else struct.unpack(">I", dol_bytes(secs, addr + off, 4))[0], addr + off)
            tn = byaddr.get(tgt)
            if tn and not tn.startswith(("fn_", "lbl_")) and sym != tn and not sym.startswith(("@", "$")):
                bad = "%s calls %s, object calls %s" % (nm, tn, sym)
                break
        if bad:
            conflicts.append((addr, bad))
        else:
            final[addr] = nm

    lines = ["fn_%08X %s  # %s%s" % (ad, nm, objfile[nm], " (static)" if objbind[nm] == 0 else "") for ad, nm in sorted(final.items())]
    lines += ["# conflict: %08X %s" % c for c in conflicts]
    text = "\n".join(lines) + "\n"
    if a.out:
        open(a.out, "w").write(text)
    else:
        sys.stdout.write(text)
    print("objects functions: %d, dol candidates: %d, matched: %d, rejected: %d"
          % (len(objfuncs), sum(len(v) for v in cand.values()), len(final), len(conflicts)), file=sys.stderr)


if __name__ == "__main__":
    main()
