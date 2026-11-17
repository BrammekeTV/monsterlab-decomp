"""Extract script callback registrations RegisterScriptFunc(name, type, fn) from the asm.

    python tools/script_funcs.py    -> one line per registration: fn  name  type  registrar

Names are read from orig/RM7P4F/sys/main.dol (needs the extracted game; never committed).
"""
import glob
import re
import struct

REG_NAMES = ("RegisterScriptFunc", "fn_801DC91C")

_dol = open("orig/RM7P4F/sys/main.dol", "rb").read()
_secs = []
for k in range(18):
    off, addr, size = (struct.unpack(">I", _dol[o + 4 * k:o + 4 * k + 4])[0] for o in (0x0, 0x48, 0x90))
    if size:
        _secs.append((addr, off, size))

_syms = {}
for line in open("config/RM7P4F/symbols.txt"):
    m = re.match(r"(\S+) = \.\w+:0x([0-9A-F]+);", line)
    if m:
        _syms[m.group(1)] = int(m.group(2), 16)


def read(addr, n):
    for a, o, sz in _secs:
        if a <= addr < a + sz:
            return _dol[o + addr - a:o + addr - a + n]
    return b""


def cstr(lbl, off):
    b = read(_syms[lbl] + off, 256)
    return b[:b.index(0)].decode("latin1")


def registrations():
    out = []
    for f in sorted(glob.glob("build/RM7P4F/asm/*_text.s")):
        t = open(f).read()
        for fm in re.finditer(r"^\.fn (\S+), \w+\n(.*?)^\.endfn", t, re.M | re.S):
            body = fm.group(2)
            if not any(f"bl {r}" in body for r in REG_NAMES):
                continue
            regs, base = {}, {}
            for line in body.splitlines():
                ins = re.sub(r"^/\*.*?\*/\s*", "", line).strip()
                m = re.match(r"lis (r\d+), (\w+)@ha$", ins)
                if m:
                    base[m.group(1)] = (m.group(2), 0)
                    continue
                m = re.match(r"addi (r\d+), (r\d+), (\w+)@l$", ins)
                if m:
                    base[m.group(1)] = (m.group(3), 0)
                    regs[m.group(1)] = ("sym", m.group(3), 0)
                    continue
                m = re.match(r"addi (r\d+), (r\d+), (-?0x[0-9a-f]+|\d+)$", ins)
                if m and m.group(2) in base:
                    s, o = base[m.group(2)]
                    regs[m.group(1)] = ("sym", s, o + int(m.group(3), 0))
                    continue
                m = re.match(r"li (r\d+), (-?0x[0-9a-f]+|\d+)$", ins)
                if m:
                    regs[m.group(1)] = ("imm", int(m.group(2), 0))
                    continue
                if ins.startswith("bl ") and ins.split()[1] in REG_NAMES:
                    n, ty, fn = regs.get("r3"), regs.get("r4"), regs.get("r5")
                    if n and fn and n[0] == "sym" and fn[0] == "sym":
                        t_ = ty[1] if ty and ty[0] == "imm" else -1
                        out.append((fn[1], cstr(n[1], n[2]), t_, fm.group(1)))
    return out


def gen(registrar, func_name, addr):
    """Print C++ for a registration function (python tools/script_funcs.py gen <asm name> <new name> <addr>)."""
    rows = [r for r in registrations() if r[3] == registrar]
    print(f"// {addr}")
    print(f"void {func_name}(void) {{")
    for fn, name, ty, _ in rows:
        print(f'    RegisterScriptFunc("{name}", 0x{ty:X}, (void*){fn});')
    print("}")


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "gen":
        gen(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        for fn, name, ty, reg in registrations():
            print(f"{fn:28} {name:40} 0x{ty:X} {reg}")
