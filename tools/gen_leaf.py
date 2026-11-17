"""Generate C++ for small straight-line leaf functions (getters, setters, constant returns,
empty stubs, simple float/bitfield accessors).

    python tools/gen_leaf.py [--max N] [--dry]

Only functions that are not decompiled yet, have no branches or calls, and use only the
instructions this script understands are generated. Each load and computation becomes its own
local, in the original order, so the C does exactly what the PowerPC code does. Output goes to
src/leaf/<object>.cpp (functions declared in script_callbacks.h keep that signature and go to
<object>_cb.cpp). Re-running regenerates those files.
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASM = os.path.join(ROOT, "build", "RM7P4F", "asm")
OUT = os.path.join(ROOT, "src", "leaf")
GAME_START, GAME_END = 0x80006B40, 0x8022FEC4

MAXN = int(sys.argv[sys.argv.index("--max") + 1]) if "--max" in sys.argv else 12
DRY = "--dry" in sys.argv

LOADS = {"lwz": "s32", "lhz": "u16", "lha": "s16", "lbz": "u8", "lfs": "f32", "lfd": "f64"}
STORES = {"stw": "u32", "sth": "u16", "stb": "u8", "stfs": "f32", "stfd": "f64"}
FBIN = {"fadds": ("+", "f32"), "fsubs": ("-", "f32"), "fmuls": ("*", "f32"), "fdivs": ("/", "f32"),
        "fadd": ("+", "f64"), "fsub": ("-", "f64"), "fmul": ("*", "f64"), "fdiv": ("/", "f64")}
IBIN = {"add": "+", "subf": "-", "and": "&", "or": "|", "xor": "^", "mullw": "*"}


class Skip(Exception):
    pass


def done_addrs():
    done = set()
    for p in glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True):
        if os.path.normpath(p).startswith(os.path.normpath(OUT)):
            continue
        for m in re.finditer(r"^// 0x([0-9A-Fa-f]{8})\s*$", open(p, encoding="utf-8").read(), re.M):
            done.add(m.group(1).upper())
    return done


def header_sigs():
    sigs = {}
    for line in open(os.path.join(ROOT, "include", "game", "script_callbacks.h"), encoding="utf-8"):
        m = re.match(r"^(.+?)\b(\w+)\((.*)\);", line)
        if m and not line.startswith("extern"):
            ret = re.sub(r"/\*.*?\*/", "", m.group(1)).strip()
            params = re.sub(r"/\*.*?\*/", "", m.group(3)).strip()
            sigs[m.group(2)] = (ret, [] if params in ("", "void") else [p.strip() for p in params.split(",")])
    return sigs


def header_names():
    names = set()
    for h in ("game/script_callbacks.h", "game/character.h", "revolution.h", "engine/math.h", "types.h"):
        t = open(os.path.join(ROOT, "include", h), encoding="utf-8").read()
        names |= set(re.findall(r"\b([A-Za-z_]\w*)\s*[\(\[;]", t))
    return names


def func_symbols():
    funcs = set()
    for line in open(os.path.join(ROOT, "config", "RM7P4F", "symbols.txt"), encoding="utf-8"):
        m = re.match(r"(\S+) = \.\w+:0x[0-9A-F]{8}; // type:function", line)
        if m:
            funcs.add(m.group(1))
    return funcs


class Gen:
    def __init__(self, name):
        self.name = name
        self.lines = []
        self.regs = {}          # reg -> (expr, type)
        self.args = {}          # reg -> usage ("ptr"/"int"/"f32"/"f64")
        self.written = set()
        self.n = 0
        self.globals = set()
        self.ptrs = set()
        self.loads = {}         # temp -> line index of a 32-bit load (retyped if used as pointer)

    def tmp(self, expr, t):
        v = f"t{self.n}"
        self.n += 1
        self.lines.append(f"    {t} {v} = {expr};")
        return v

    def get(self, r, use):
        if r in self.regs:
            return self.regs[r]
        if r in ("r0", "r1", "r2", "r13") or (r.startswith("r") and int(r[1:]) > 10) or \
           (r.startswith("f") and not 1 <= int(r[1:]) <= 8):
            raise Skip("reads " + r)
        prev = self.args.get(r)
        if r.startswith("r"):
            self.args[r] = "ptr" if use == "ptr" or prev == "ptr" else "int"
            e = (f"a{r[1:]}", "ARG:" + r)
        else:
            self.args[r] = "f64" if use == "f64" or prev == "f64" else "f32"
            e = (f"f{r[1:]}", "FARG")
        return e

    def ptr(self, r):
        e, t = self.get(r, "ptr")
        if t in ("u8*",):
            return e
        if t.startswith("ARG"):
            self.args[t[4:]] = "ptr"
            return f"((u8*)(uintptr_t)({e}))"
        if e in self.loads:
            i = self.loads.pop(e)
            self.lines[i] = self.lines[i].replace(f"s32 {e} = *(s32*)", f"u8* {e} = *(u8**)")
            for reg, (x, tt) in list(self.regs.items()):
                if x == e:
                    self.regs[reg] = (e, "u8*")
            self.ptrs.add(e)
            return e
        if e in self.ptrs:
            return e
        if t.startswith("f"):
            raise Skip("float used as pointer")
        return f"(u8*)(uintptr_t)({e})"

    def val(self, r, ctype="u32"):
        e, t = self.get(r, "int")
        if e in self.loads:
            self.loads.pop(e)
        if t == "u8*" or t.startswith("ARG") or e in self.ptrs:
            return f"(u32)(uintptr_t)({e})"
        if t.startswith("f"):
            raise Skip("float used as int")
        return f"(u32)({e})"

    def fval(self, r):
        e, t = self.get(r, "f")
        if t in ("FARG", "f32", "f64"):
            return e
        raise Skip("int used as float")

    def set(self, r, e, t):
        self.regs[r] = (e, t)
        self.written.add(r)

    def addr(self, mem):
        m = re.match(r"(-?0x[0-9a-f]+|-?\d+)\((r\d+)\)$", mem)
        if m:
            return f"{self.ptr(m.group(2))} + {int(m.group(1), 0)}"
        m = re.match(r"(\w+)@sda21\(r0\)$", mem)
        if m:
            self.globals.add(m.group(1))
            return f"(u8*){m.group(1)}"
        raise Skip("addr " + mem)

    def imm(self, s):
        m = re.match(r"(\w+)@sda21$", s)
        if m:
            self.globals.add(m.group(1))
            return None, m.group(1)
        return int(s, 0), None

    def step(self, op, a):
        if op in LOADS:
            t = LOADS[op]
            v = self.tmp(f"*({t}*)({self.addr(a[1])})", t)
            if op == "lwz":
                self.loads[v] = len(self.lines) - 1
            self.set(a[0], v, t)
        elif op in STORES:
            t = STORES[op]
            v = self.fval(a[0]) if t.startswith("f") else self.val(a[0])
            self.lines.append(f"    *({t}*)({self.addr(a[1])}) = ({t})({v});")
        elif op == "li":
            v, sym = self.imm(a[1])
            if sym:
                self.set(a[0], f"(u8*){sym}", "u8*")
            else:
                self.set(a[0], str(v), "s32")
        elif op == "lis":
            m = re.match(r"(\w+)@ha$", a[1])
            if m:
                self.set(a[0], ("HA", m.group(1)), "HA")
            else:
                self.set(a[0], hex((int(a[1], 0) << 16) & 0xFFFFFFFF), "u32")
        elif op in ("addi", "subi", "addis", "subis"):
            m = re.match(r"(\w+)@l$", a[2])
            if m:
                if self.regs.get(a[1], (None, None))[1] != "HA":
                    raise Skip("@l without @ha")
                self.globals.add(m.group(1))
                self.set(a[0], f"(u8*){m.group(1)}", "u8*")
                return
            k = int(a[2], 0)
            if op in ("subi", "subis"):
                k = -k
            if op in ("addis", "subis"):
                k <<= 16
            if a[1] == "r0":
                raise Skip("addi r0 base")
            e, t = self.get(a[1], "int")
            if t == "HA":
                raise Skip("ha arith")
            if (t == "u8*" or t.startswith("ARG")) and op in ("addi", "subi"):
                self.set(a[0], self.tmp(f"{self.ptr(a[1])} + {k}", "u8*"), "u8*")
            else:
                self.set(a[0], self.tmp(f"{self.val(a[1])} + {k}u" if k >= 0 else f"{self.val(a[1])} - {-k}u", "u32"), "u32")
        elif op == "ori" or op == "oris" or op == "xori" or op == "andi.":
            k = int(a[2], 0) << (16 if op == "oris" else 0)
            o = {"ori": "|", "oris": "|", "xori": "^", "andi.": "&"}[op]
            self.set(a[0], self.tmp(f"{self.val(a[1])} {o} {hex(k)}u", "u32"), "u32")
        elif op == "mr":
            self.set(a[0], *self.get(a[1], self.args.get(a[1])))
        elif op == "fmr":
            self.set(a[0], self.fval(a[1]), "f64")
        elif op == "frsp":
            self.set(a[0], self.tmp(f"(f32)({self.fval(a[1])})", "f32"), "f32")
        elif op in ("fneg", "fabs"):
            e = self.fval(a[1])
            self.set(a[0], self.tmp(f"-({e})" if op == "fneg" else f"(({e}) < 0 ? -({e}) : ({e}))", "f64"), "f64")
        elif op in FBIN:
            o, t = FBIN[op]
            self.set(a[0], self.tmp(f"({t})({self.fval(a[1])} {o} {self.fval(a[2])})", t), t)
        elif op in ("fmadds", "fmsubs", "fmadd", "fmsub"):
            t = "f32" if op.endswith("s") else "f64"
            o = "+" if "madd" in op else "-"
            self.set(a[0], self.tmp(f"({t})({self.fval(a[1])} * {self.fval(a[2])} {o} {self.fval(a[3])})", t), t)
        elif op in IBIN:
            x, y = self.val(a[1]), self.val(a[2])
            e = f"{y} - {x}" if op == "subf" else f"{x} {IBIN[op]} {y}"
            self.set(a[0], self.tmp(e, "u32"), "u32")
        elif op == "neg":
            self.set(a[0], self.tmp(f"0u - {self.val(a[1])}", "u32"), "u32")
        elif op == "mulli":
            self.set(a[0], self.tmp(f"(u32)((s32){self.val(a[1])} * {int(a[2], 0)})", "u32"), "u32")
        elif op in ("extsb", "extsh"):
            t = "s8" if op == "extsb" else "s16"
            self.set(a[0], self.tmp(f"(u32)(s32)({t}){self.val(a[1])}", "u32"), "u32")
        elif op == "srawi":
            self.set(a[0], self.tmp(f"(u32)((s32){self.val(a[1])} >> {int(a[2], 0)})", "u32"), "u32")
        elif op in ("rlwinm", "slwi", "srwi", "clrlwi", "clrrwi", "extrwi", "rotlwi", "rotrwi"):
            k = [int(x, 0) for x in a[2:]]
            sh, mb, me = {
                "rlwinm": lambda: k,
                "slwi": lambda: (k[0], 0, 31 - k[0]),
                "srwi": lambda: ((32 - k[0]) & 31, k[0], 31),
                "clrlwi": lambda: (0, k[0], 31),
                "clrrwi": lambda: (0, 0, 31 - k[0]),
                "extrwi": lambda: ((k[1] + k[0]) & 31, 32 - k[0], 31),
                "rotlwi": lambda: (k[0], 0, 31),
                "rotrwi": lambda: ((32 - k[0]) & 31, 0, 31),
            }[op]()
            mask = 0
            i = mb
            while True:
                mask |= 1 << (31 - i)
                if i == me:
                    break
                i = (i + 1) & 31
            v = self.val(a[1])
            rot = v if sh == 0 else f"(({v} << {sh}) | ({v} >> {32 - sh}))"
            self.set(a[0], self.tmp(f"{rot} & {hex(mask)}u", "u32"), "u32")
        elif op == "cntlzw":
            raise Skip("cntlzw")
        else:
            raise Skip("op " + op)


def gen_function(name, ins, sig):
    g = Gen(name)
    body = ins[:-1]
    if not ins or ins[-1] != ("blr", []):
        raise Skip("not ending in blr")
    for op, a in body:
        g.step(op, a)

    def argtype(r, use):
        if r.startswith("f"):
            return use
        return "u8*" if use == "ptr" else "s32"

    rargs = sorted((r for r in g.args if r.startswith("r")), key=lambda r: int(r[1:]))
    fargs = sorted((r for r in g.args if r.startswith("f")), key=lambda r: int(r[1:]))
    nr = int(rargs[-1][1:]) - 2 if rargs else 0
    nf = int(fargs[-1][1:]) if fargs else 0
    ret3 = "r3" in g.written
    retf = "f1" in g.written
    if ret3 and retf:
        raise Skip("two return regs")

    if sig:
        rett, params = sig
        ints = [p for p in params if not re.match(r"^(f32|f64|float|double)\b", p)]
        flts = [p for p in params if re.match(r"^(f32|f64|float|double)\b", p)]
        if nr > len(ints) or nf > len(flts):
            raise Skip("header has fewer params")
        decl_params, ri, fi = [], 3, 1
        for p in params:
            pt = re.sub(r"\s*\b\w+$", "", p).strip() or p
            if re.match(r"^(f32|f64|float|double)\b", p):
                decl_params.append(f"{pt} f{fi}")
                fi += 1
            else:
                decl_params.append(f"{pt} a{ri}")
                ri += 1
        head = f"{rett} {name}({', '.join(decl_params) or 'void'})"
    else:
        ps = [f"{argtype(f'r{i}', g.args.get(f'r{i}', 'int'))} a{i}" for i in range(3, 3 + nr)]
        ps += [f"{g.args.get(f'f{i}', 'f32')} f{i}" for i in range(1, 1 + nf)]
        if ret3:
            rett = g.regs["r3"][1]
            if rett.startswith("ARG"):
                rett = "u8*" if g.args.get(rett[4:]) == "ptr" else "s32"
            rett = {"HA": None}.get(rett, rett)
            if rett is None:
                raise Skip("returns ha")
        elif retf:
            rett = "f32" if g.regs["f1"][1] in ("f32", "FARG") else "f64"
        else:
            rett = "void"
        head = f'extern "C" {rett} {name}({", ".join(ps) or "void"})'

    out = [head + " {"] + g.lines
    if rett != "void":
        if ret3:
            e, t = g.regs["r3"]
        elif retf:
            e, t = g.regs["f1"]
        else:
            raise Skip("header returns a value the code doesn't set")
        if "*" in rett and t not in ("u8*",):
            out.append(f"    return ({rett})(uintptr_t)({e});")
        elif "*" not in rett and (t == "u8*" or t.startswith("ARG")) and not rett.startswith("f"):
            out.append(f"    return ({rett})(uintptr_t)({e});")
        else:
            out.append(f"    return ({rett})({e});")
    out.append("}")
    return out, g.globals


def parse(body):
    ins = []
    for m in re.finditer(r"^/\* ([0-9A-F]{8}) [0-9A-F]{8}  [0-9A-F ]{11} \*/\t(\S+)\s*(.*)$", body, re.M):
        a = [x.strip() for x in m.group(3).split(",")] if m.group(3) else []
        ins.append((m.group(1), m.group(2), a))
    return ins


def main():
    done = done_addrs()
    sigs = header_sigs()
    hnames = header_names()
    funcs = func_symbols()
    total, skipped = 0, {}
    files = {}
    for f in sorted(glob.glob(os.path.join(ASM, "*_text.s"))):
        obj = os.path.basename(f)[:-2]
        t = open(f, encoding="utf-8", errors="replace").read()
        for m in re.finditer(r"^\.fn (\S+), \w+\n(.*?)^\.endfn", t, re.M | re.S):
            name, body = m.group(1), m.group(2)
            if ".L_" in body:
                continue
            ins = parse(body)
            if not ins or len(ins) > MAXN:
                continue
            addr = int(ins[0][0], 16)
            if not GAME_START <= addr < GAME_END or ins[0][0] in done:
                continue
            try:
                code, globs = gen_function(name, [(o, a) for _, o, a in ins], sigs.get(name))
            except Skip as e:
                skipped[str(e).split()[0]] = skipped.get(str(e).split()[0], 0) + 1
                continue
            key = obj + ("_cb" if name in sigs else "")
            files.setdefault(key, []).append((ins[0][0], name, code, globs))
            total += 1

    print(f"generated {total} functions in {len(files)} files")
    print("skipped:", dict(sorted(skipped.items(), key=lambda x: -x[1])[:12]))
    if DRY:
        return
    os.makedirs(OUT, exist_ok=True)
    for old in glob.glob(os.path.join(OUT, "*.cpp")):
        os.remove(old)
    for key, fns in files.items():
        cb = key.endswith("_cb")
        defined = {n for _, n, _, _ in fns}
        lines = ["// Generated by tools/gen_leaf.py: small leaf functions, translated instruction by",
                 "// instruction. Do not edit by hand; decompile into a real source file instead and",
                 "// the generator will leave the function out on the next run.",
                 "#include <stdint.h>", "", '#include "types.h"']
        if cb:
            lines.append('#include "game/script_callbacks.h"')
        lines.append("")
        globs = sorted(set().union(*[g for _, _, _, g in fns]))
        heads = {n: c[0][:-2] for _, n, c, _ in fns}
        for gname in globs:
            if gname in defined:
                if gname in funcs and not (cb and gname in hnames):
                    lines.append(heads[gname] + ";")
                continue
            if cb and gname in hnames:
                continue
            if gname in funcs:
                lines.append(f'extern "C" void {gname}(void);')
            else:
                lines.append(f"extern u8 {gname}[];")
        if globs:
            lines.append("")
        for addr, name, code, _ in sorted(fns):
            lines.append(f"// 0x{addr}")
            lines += code
            lines.append("")
        src = "\n".join(lines)
        # function symbols are referenced by address
        for gname in globs:
            if gname in funcs:
                src = src.replace(f"(u8*){gname}", f"(u8*)(uintptr_t)&{gname}")
        open(os.path.join(OUT, key + ".cpp"), "w", encoding="utf-8").write(src)


if __name__ == "__main__":
    main()
