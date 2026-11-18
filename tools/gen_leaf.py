"""Generate C++ for small leaf functions: getters, setters, constant returns, empty stubs,
simple float/bitfield accessors, compare-and-return functions and forwarding wrappers.

    python tools/gen_leaf.py [--max N] [--dry]

Only functions that are not decompiled yet and use only the instructions this script
understands are generated. Each load and computation becomes its own local, in the original
order, so the C does exactly what the PowerPC code does. Control flow is limited to forward
branches: integer/float compares (`cmpwi`/`cmplw`/`fcmpo`, `cror` <=/>=, record forms like
`andi.`), conditional returns (`beqlr`, ...), forward branches (the branch target is emitted
inside the `if`, so code reached from two places is duplicated) and a tail call `b Target`
(written as `return Target(...)`). The `cntlzw; srwi 5` idiom becomes `x == 0`.

Tail-call targets use, in order: the script_callbacks.h signature, an `extern "C"` prototype
or definition found in src/ or include/, the head of another generated function, or else an
argument list inferred from the registers written before the call (marked "inferred").

Output goes to src/leaf/<object>.cpp (functions declared in script_callbacks.h keep that
signature and go to <object>_cb.cpp). Re-running regenerates those files.
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASM = os.path.join(ROOT, "build", "RM7P4F", "asm")
OUT = os.path.join(ROOT, "src", "leaf")
GAME_START, GAME_END = 0x80006B40, 0x8022FEC4

MAXN = int(sys.argv[sys.argv.index("--max") + 1]) if "--max" in sys.argv else 12
DRY = "--dry" in sys.argv
FUNCS = {}              # function symbol -> address
MAX_STEPS = 120         # instructions emitted per function, counting duplicated paths

LOADS = {"lwz": "s32", "lhz": "u16", "lha": "s16", "lbz": "u8", "lfs": "f32", "lfd": "f64"}
STORES = {"stw": "u32", "sth": "u16", "stb": "u8", "stfs": "f32", "stfd": "f64"}
FBIN = {"fadds": ("+", "f32"), "fsubs": ("-", "f32"), "fmuls": ("*", "f32"), "fdivs": ("/", "f32"),
        "fadd": ("+", "f64"), "fsub": ("-", "f64"), "fmul": ("*", "f64"), "fdiv": ("/", "f64")}
IBIN = {"add": "+", "subf": "-", "and": "&", "or": "|", "xor": "^", "mullw": "*"}
CMPS = {"cmpwi": "s", "cmpw": "s", "cmplwi": "u", "cmplw": "u", "fcmpu": "f", "fcmpo": "f"}
FLIP = {"==": "!=", "<": ">=", ">": "<=", "<=": ">", ">=": "<"}
BASIC = {"void", "u8", "s8", "u16", "s16", "u32", "s32", "f32", "f64", "BOOL", "char", "int",
         "float", "double", "bool"}
FLOAT_T = re.compile(r"^(f32|f64|float|double)\b")


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


def param_type(p):
    return re.sub(r"\s*\b\w+$", "", p).strip() or p


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
    funcs = {}
    for line in open(os.path.join(ROOT, "config", "RM7P4F", "symbols.txt"), encoding="utf-8"):
        m = re.match(r"(\S+) = \.\w+:0x([0-9A-F]{8}); // type:function", line)
        if m:
            funcs[m.group(1)] = int(m.group(2), 16)
    return funcs


def norm_type(t):
    """A type usable in a leaf file (only types.h): unknown pointers become u8*, else None."""
    t = re.sub(r"\b(const|struct|volatile)\b", "", t)
    t = re.sub(r"\s*\*\s*", "*", t).strip()
    base = t.rstrip("*")
    if base in ("s64", "u64", "long long"):
        return None
    if base in BASIC:
        return t
    if "*" in t and re.fullmatch(r"\w+", base):
        return "u8*"
    return None


def norm_proto(ret, params):
    """(ret, [param types]) with types.h types only, or None."""
    r = norm_type(ret)
    if r is None:
        return None
    out = []
    for p in params:
        p = p.split("=")[0].strip()
        if p == "..." or "(" in p:
            return None
        if "[" in p:
            p = p[:p.index("[")].strip() + "*"
            p = re.sub(r"\s*\b\w+\*$", "*", p)
            t = norm_type(p)
        else:
            t = norm_type(param_type(p))
        if t is None or t == "void":
            return None
        out.append(t)
    return r, out


def split_params(s):
    s = re.sub(r"/\*.*?\*/", "", s).strip()
    return [] if s in ("", "void") else [p.strip() for p in s.split(",")]


PROTO_RE = re.compile(r'^\s*(?:extern "C"\s+)?(?:static\s+)?(?:inline\s+)?([A-Za-z_][\w \t\*]*?)\s*\b(\w+)\s*'
                      r'\(([^()]*)\)\s*(?:;|\{)')


def source_protos(funcs):
    """extern "C" prototypes / definitions of game functions in src/ and include/."""
    protos = {}
    paths = glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True) + \
        glob.glob(os.path.join(ROOT, "include", "**", "*.h"), recursive=True)
    for p in sorted(paths):
        if os.path.normpath(p).startswith(os.path.normpath(OUT)):
            continue
        for line in open(p, encoding="utf-8", errors="replace"):
            m = PROTO_RE.match(line)
            if not m or m.group(2) not in funcs or m.group(2) in protos:
                continue
            if re.search(r"\b(return|else|case|new|delete|goto)\b", m.group(1)):
                continue
            pr = norm_proto(m.group(1).strip(), split_params(m.group(3)))
            if pr:
                protos[m.group(2)] = pr
    return protos


PRIM_RE = re.compile(r"^(?:[us](?:8|16|32|64)|f32|f64|BOOL|void\s*\*|u8\s*\*)$")
GLOB_RE = re.compile(r"^(?:extern\s+)?([A-Za-z_][\w \t\*]*?)\s*\b((?:g[A-Z]|lbl_)\w*)\s*(?:;|=)")


def global_types():
    """Scalar/pointer types of globals declared or defined in hand-written sources, so the leaf
    files declare them with the same type (the linker needs identical C++ decorated names)."""
    types = {}
    for p in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        if os.path.normpath(p).startswith(os.path.normpath(OUT)):
            continue
        for line in open(p, encoding="utf-8", errors="replace"):
            if line[:1] in " 	/#":
                continue
            m = GLOB_RE.match(line)
            if m and PRIM_RE.match(re.sub(r"\s+", " ", m.group(1)).replace(" *", "*").strip()):
                types.setdefault(m.group(2), re.sub(r"\s+", " ", m.group(1)).replace(" *", "*").strip())
    return types


def head_proto(head):
    m = re.match(r'^extern "C" (.+?)\b(\w+)\((.*)\)$', head)
    return norm_proto(m.group(1).strip(), split_params(m.group(3))) if m else None


class Gen:
    def __init__(self, name, cb, sigs, protos, hnames):
        self.name = name
        self.sig = sigs.get(name) if cb else None
        self.cb = cb
        self.sigs, self.protos, self.hnames = sigs, protos, hnames
        self.lines = []
        self.ind = "    "
        self.regs = {}          # reg -> (expr, type)
        self.args = {}          # reg -> usage ("ptr"/"int"/"f32"/"f64")
        self.written = set()
        self.pending = set()    # written regs not read since (outgoing call arguments)
        self.cr = {}            # cr field -> compare
        self.n = 0
        self.globals = set()
        self.ptrs = set()
        self.loads = {}         # temp -> line index of a 32-bit load (retyped if used as pointer)
        self.calls = {}         # tail-call target -> (ret, [types]) to declare, or None (header)
        self.exits = []         # return/tail-call placeholders in self.lines
        self.steps = 0
        self.inferred = False

    # ---- state -------------------------------------------------------------------------
    def save(self):
        return dict(self.regs), set(self.written), set(self.pending), dict(self.cr)

    def restore(self, st):
        self.regs, self.written, self.pending, self.cr = dict(st[0]), set(st[1]), set(st[2]), dict(st[3])

    def tmp(self, expr, t):
        v = f"t{self.n}"
        self.n += 1
        self.lines.append(f"{self.ind}{t} {v} = {expr};")
        return v

    def get(self, r, use):
        if r in self.regs:
            self.pending.discard(r)
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
        if t == "CLZ":
            raise Skip("cntlzw")
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
        if t.startswith("f") or t == "FARG":
            raise Skip("float used as pointer")
        return f"(u8*)(uintptr_t)({e})"

    def valx(self, e, t):
        if t == "CLZ" or t == "HA":
            raise Skip("cntlzw" if t == "CLZ" else "ha arith")
        if e in self.loads:
            self.loads.pop(e)
        if t == "u8*" or t.startswith("ARG") or e in self.ptrs:
            return f"(u32)(uintptr_t)({e})"
        if t.startswith("f") or t == "FARG":
            raise Skip("float used as int")
        return f"(u32)({e})"

    def val(self, r, ctype="u32"):
        return self.valx(*self.get(r, "int"))

    def fval(self, r):
        e, t = self.get(r, "f")
        if t in ("FARG", "f32", "f64"):
            return e
        raise Skip("int used as float")

    def set(self, r, e, t):
        self.regs[r] = (e, t)
        self.written.add(r)
        self.pending.add(r)

    def addr(self, mem):
        m = re.match(r"(-?0x[0-9a-f]+|-?\d+)\((r\d+)\)$", mem)
        if m:
            return f"{self.ptr(m.group(2))} + {int(m.group(1), 0)}"
        m = re.match(r"(\w+)@sda21\(r0\)$", mem)
        if m:
            self.globals.add(m.group(1))
            return f"(u8*)&{m.group(1)}"
        raise Skip("addr " + mem)

    def imm(self, s):
        m = re.match(r"(\w+)@sda21$", s)
        if m:
            self.globals.add(m.group(1))
            return None, m.group(1)
        return int(s, 0), None

    # ---- straight-line instructions ------------------------------------------------------
    def step(self, op, a):
        if op.endswith(".") and op != "andi.":
            self.step(op[:-1], a)
            self.cr["cr0"] = {"k": "s", "x": self.regs[a[0]], "y": 0, "eq": "=="}
            return
        if op in LOADS:
            t = LOADS[op]
            v = self.tmp(f"*({t}*)({self.addr(a[1])})", t)
            if op == "lwz":
                self.loads[v] = len(self.lines) - 1
            self.set(a[0], v, t)
        elif op in STORES:
            t = STORES[op]
            v = self.fval(a[0]) if t.startswith("f") else self.val(a[0])
            self.lines.append(f"{self.ind}*({t}*)({self.addr(a[1])}) = ({t})({v});")
        elif op == "li":
            v, sym = self.imm(a[1])
            if sym:
                self.set(a[0], f"(u8*)&{sym}", "u8*")
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
                self.set(a[0], f"(u8*)&{m.group(1)}", "u8*")
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
        elif op == "andi.":
            self.set(a[0], self.tmp(f"{self.val(a[1])} & {hex(int(a[2], 0))}u", "u32"), "u32")
            self.cr["cr0"] = {"k": "s", "x": self.regs[a[0]], "y": 0, "eq": "=="}
        elif op == "ori" or op == "oris" or op == "xori":
            k = int(a[2], 0) << (16 if op == "oris" else 0)
            o = {"ori": "|", "oris": "|", "xori": "^"}[op]
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
            src = self.regs.get(a[1])
            if src and src[1] == "CLZ":
                # cntlzw x; srwi 5 -> (x == 0)
                if not (sh == 27 and me == 31 and mb >= 5):
                    raise Skip("cntlzw")
                self.pending.discard(a[1])
                self.set(a[0], self.tmp(f"(u32)({self.zero_expr(src[0])} == 0)", "u32"), "u32")
                return
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
            e, t = self.get(a[1], "int")
            if t.startswith("f") or t in ("FARG", "CLZ", "HA"):
                raise Skip("cntlzw")
            self.set(a[0], (e, t), "CLZ")
        elif op == "nop":
            pass
        else:
            raise Skip("op " + op)

    def zero_expr(self, x):
        """Operand of an ==/!= 0 test, without forcing a pointer/int interpretation."""
        e, t = x
        if t.startswith("f") or t in ("FARG", "CLZ", "HA"):
            raise Skip("cmp type")
        return e

    # ---- control flow --------------------------------------------------------------------
    def compare(self, op, a):
        cr = "cr0"
        if a[0].startswith("cr"):
            cr, a = a[0], a[1:]
        k = CMPS[op]
        if k == "f":
            self.cr[cr] = {"k": "f", "x": self.fval(a[0]), "y": self.fval(a[1]), "eq": "=="}
        else:
            x = self.get(a[0], "int")
            y = int(a[1], 0) if op.endswith("i") else self.get(a[1], "int")
            self.cr[cr] = {"k": k, "x": x, "y": y, "eq": "=="}

    def cond(self, cr, code):
        c = self.cr.get(cr)
        if c is None:
            raise Skip("no compare")
        bit, true = {"eq": ("eq", 1), "ne": ("eq", 0), "lt": ("lt", 1), "ge": ("lt", 0),
                     "gt": ("gt", 1), "le": ("gt", 0)}[code]
        o = {"lt": "<", "gt": ">", "eq": c["eq"]}[bit]
        if c["k"] == "f":
            x, y = c["x"], c["y"]
            return f"{x} {o} {y}" if true else f"!({x} {o} {y})"
        if not true:
            o = FLIP[o]
        if c["y"] == 0 and o in ("==", "!=") and c["k"] in ("s", "u"):
            return f"{self.zero_expr(c['x'])} {o} 0"
        if c["k"] == "s":
            x = f"(s32){self.valx(*c['x'])}"
            y = str(c["y"]) if isinstance(c["y"], int) else f"(s32){self.valx(*c['y'])}"
        else:
            x = self.valx(*c["x"])
            y = f"{hex(c['y'] & 0xFFFFFFFF)}u" if isinstance(c["y"], int) else self.valx(*c["y"])
        return f"{x} {o} {y}"

    def cror(self, a):
        if a == ["eq", "lt", "eq"] and "cr0" in self.cr:
            self.cr["cr0"] = dict(self.cr["cr0"], eq="<=")
        elif a == ["eq", "gt", "eq"] and "cr0" in self.cr:
            self.cr["cr0"] = dict(self.cr["cr0"], eq=">=")
        else:
            raise Skip("cror")

    def ret(self):
        r3, f1 = self.regs.get("r3"), self.regs.get("f1")
        self.exits.append(len(self.lines))
        self.lines.append(("RET", self.ind, r3, f1))

    def arg_expr(self, r, ty):
        if FLOAT_T.match(ty):
            return f"({ty})({self.fval(r)})"
        if "*" in ty:
            p = self.ptr(r)
            return p if ty == "u8*" else f"({ty})({p})"
        return f"({ty})({self.val(r)})"

    def tail(self, tgt):
        if self.cb and tgt in self.sigs:
            ret, params = self.sigs[tgt]
            types = [param_type(p) for p in params]
            decl = None
        elif self.cb and tgt in self.hnames:
            raise Skip("tail to header fn")
        elif tgt in self.protos:
            ret, types = self.protos[tgt]
            decl = (ret, types)
        elif not GAME_START <= FUNCS.get(tgt, 0) < GAME_END:
            raise Skip("tail to SDK fn without prototype")
        else:
            # infer: arguments are the registers written and not read since, plus untouched
            # incoming registers below the highest of them
            outs = [r for r in self.pending if re.fullmatch(r"r([3-9]|10)", r)]
            fouts = [r for r in self.pending if re.fullmatch(r"f[1-8]", r)]
            n = max((int(r[1:]) for r in outs), default=2) - 2
            if self.sig:    # the wrapper's own arguments may be passed through untouched
                n = max(n, len([p for p in self.sig[1] if not FLOAT_T.match(p)]))
            if n == 0 and not self.written:
                raise Skip("tail args unknown")
            nf = max((int(r[1:]) for r in fouts), default=0)
            types = []
            for i in range(3, 3 + n):
                r = f"r{i}"
                e, t = self.regs.get(r) or self.get(r, "int")
                if t.startswith("f") or t in ("FARG", "CLZ", "HA"):
                    raise Skip("tail arg type")
                isptr = t == "u8*" or e in self.ptrs or (t.startswith("ARG") and self.args.get(t[4:]) == "ptr")
                types.append("u8*" if isptr else "s32")
            for i in range(1, 1 + nf):
                e, t = self.regs.get(f"f{i}") or self.get(f"f{i}", "f")
                types.append("f64" if t == "f64" else "f32")
            ret = "INFER"
            decl = (ret, types)
            self.inferred = True
        ri, fi, args = 3, 1, []
        for ty in types:
            if FLOAT_T.match(ty):
                args.append(self.arg_expr(f"f{fi}", ty))
                fi += 1
            else:
                if ri > 10:
                    raise Skip("too many args")
                args.append(self.arg_expr(f"r{ri}", ty))
                ri += 1
        if tgt in self.calls and self.calls[tgt] != decl:
            raise Skip("two protos")
        self.calls[tgt] = decl
        self.exits.append(len(self.lines))
        self.lines.append(("TAIL", self.ind, f"{tgt}({', '.join(args)})", ret))

    def run(self, items, labels, i):
        while i < len(items):
            it = items[i]
            if it[0] == "label":
                i += 1
                continue
            op, a = it[1], it[2]
            self.steps += 1
            if self.steps > MAX_STEPS:
                raise Skip("too many paths")
            if op == "blr":
                self.ret()
                return
            if op == "b":
                if a[0].startswith(".L_"):
                    if a[0] not in labels:
                        raise Skip("branch out of function")
                    j = labels[a[0]]
                    if j <= i:
                        raise Skip("loop")
                    i = j
                    continue
                self.tail(a[0])
                return
            m = re.fullmatch(r"b(eq|ne|lt|ge|gt|le)(lr)?", op)
            if m:
                cr = "cr0"
                if a and a[0].startswith("cr"):
                    cr, a = a[0], a[1:]
                c = self.cond(cr, m.group(1))
                if not m.group(2):
                    if not a or not a[0].startswith(".L_"):
                        raise Skip("conditional tail call")
                    if a[0] not in labels:
                        raise Skip("branch out of function")
                    j = labels[a[0]]
                    if j <= i:
                        raise Skip("loop")
                st = self.save()
                self.lines.append(f"{self.ind}if ({c}) {{")
                self.ind += "    "
                if m.group(2):
                    self.ret()
                else:
                    self.run(items, labels, j)
                self.ind = self.ind[:-4]
                self.lines.append(f"{self.ind}}}")
                self.restore(st)
                i += 1
                continue
            if op in CMPS:
                self.compare(op, a)
            elif op == "cror":
                self.cror(a)
            else:
                self.step(op, a)
            i += 1
        raise Skip("falls off the end")


def conv_ret(rett, e, t):
    if "*" in rett and t != "u8*":
        return f"({rett})(uintptr_t)({e})"
    if "*" not in rett and (t == "u8*" or t.startswith("ARG")) and not FLOAT_T.match(rett):
        return f"({rett})(uintptr_t)({e})"
    return f"({rett})({e})"


def gen_function(name, items, sig, cb, sigs, protos, hnames):
    g = Gen(name, cb, sigs, protos, hnames)
    labels = {it[1]: k for k, it in enumerate(items) if it[0] == "label"}
    g.run(items, labels, 0)

    rets = [g.lines[k] for k in g.exits if g.lines[k][0] == "RET"]
    tails = [g.lines[k] for k in g.exits if g.lines[k][0] == "TAIL"]
    ret3 = [r[2] for r in rets if r[2] is not None]
    retf = [r[3] for r in rets if r[3] is not None]
    tret = {t[3] for t in tails if t[3] != "INFER"}
    if len(tret) > 1:
        raise Skip("tail rets differ")
    tret = tret.pop() if tret else None

    if sig:
        rett = sig[0]
    elif tret is not None:
        rett = tret
    elif ret3 and retf:
        raise Skip("two return regs")
    elif ret3:
        ts = {t for _, t in ret3}
        if "HA" in ts or "CLZ" in ts:
            raise Skip("returns ha")
        if any(t.startswith("f") or t == "FARG" for t in ts):
            raise Skip("float in r3")
        ts = {("u8*" if g.args.get(t[4:]) == "ptr" else "s32") if t.startswith("ARG") else t for t in ts}
        ts = {"u8*" if any(e in g.ptrs for e, _ in ret3) else t for t in ts}
        rett = next(iter(ts)) if len(ts) == 1 else next((t for t in ("u8*", "u32", "s32") if t in ts), "s32")
    elif retf:
        rett = "f32" if all(t in ("f32", "FARG") for _, t in retf) else "f64"
    else:
        rett = "void"
    isf = bool(FLOAT_T.match(rett))
    if rett != "void" and not (ret3 if not isf else retf) and tret in (None, "void") and \
       not (tails and any(t[3] == "INFER" for t in tails)):
        raise Skip("header returns a value the code doesn't set")
    if tret == "void" and rett != "void":
        raise Skip("void tail in value function")
    for tgt, d in g.calls.items():
        if d and d[0] == "INFER":
            r = norm_type(rett)
            if r is None:
                raise Skip("inferred ret type")
            g.calls[tgt] = (r, d[1])

    # render exits
    for k in g.exits:
        kind, ind, x, y = g.lines[k]
        last = k == len(g.lines) - 1 and ind == "    "
        if kind == "RET":
            if rett == "void":
                g.lines[k] = None if last else f"{ind}return;"
                continue
            v = y if isf else x
            if v is None:     # returns its own (untouched) argument
                r = "f1" if isf else "r3"
                g.args.setdefault(r, "f32" if isf else "int")
                v = ("f1", "FARG") if isf else ("a3", "ARG:r3")
            e, t = v
            if not isf and (t.startswith("f") or t == "FARG"):
                raise Skip("float in r3")
            if isf and not (t.startswith("f") or t == "FARG"):
                raise Skip("int in f1")
            if t in ("HA", "CLZ"):
                raise Skip("returns ha")
            g.lines[k] = f"{ind}return {conv_ret(rett, e, 'u8*' if e in g.ptrs else t)};"
        else:
            tr = rett if y == "INFER" else y
            if rett == "void":
                g.lines[k] = f"{ind}{x};" + ("" if last else f"\n{ind}return;")
            elif tr == "void":
                raise Skip("void tail in value function")
            else:
                g.lines[k] = f"{ind}return {conv_ret(rett, x, 'u8*' if '*' in tr else tr)};"
    body = [l for l in g.lines if l is not None]

    def argtype(r, use):
        if r.startswith("f"):
            return use
        return "u8*" if use == "ptr" else "s32"

    rargs = sorted((r for r in g.args if r.startswith("r")), key=lambda r: int(r[1:]))
    fargs = sorted((r for r in g.args if r.startswith("f")), key=lambda r: int(r[1:]))
    nr = int(rargs[-1][1:]) - 2 if rargs else 0
    nf = int(fargs[-1][1:]) if fargs else 0

    if sig:
        params = sig[1]
        ints = [p for p in params if not FLOAT_T.match(p)]
        flts = [p for p in params if FLOAT_T.match(p)]
        if nr > len(ints) or nf > len(flts):
            raise Skip("header has fewer params")
        decl_params, ri, fi = [], 3, 1
        for p in params:
            pt = param_type(p)
            if FLOAT_T.match(p):
                decl_params.append(f"{pt} f{fi}")
                fi += 1
            else:
                decl_params.append(f"{pt} a{ri}")
                ri += 1
        head = f"{rett} {name}({', '.join(decl_params) or 'void'})"
    else:
        ps = [f"{argtype(f'r{i}', g.args.get(f'r{i}', 'int'))} a{i}" for i in range(3, 3 + nr)]
        ps += [f"{g.args.get(f'f{i}', 'f32')} f{i}" for i in range(1, 1 + nf)]
        head = f'extern "C" {rett} {name}({", ".join(ps) or "void"})'

    out = [head + " {"]
    if g.inferred:
        out.append("    // tail-call arguments inferred from the registers set before the call")
    out += body + ["}"]
    return out, g.globals, g.calls


def parse(body):
    items = []
    for m in re.finditer(r"^(?:(\.L_[0-9A-F]+):|/\* ([0-9A-F]{8}) [0-9A-F]{8}  [0-9A-F ]{11} \*/\t(\S+)[ \t]*([^\n]*))$",
                         body, re.M):
        if m.group(1):
            items.append(("label", m.group(1)))
            continue
        a = [x.strip() for x in m.group(4).split(",")] if m.group(4) else []
        items.append(("ins", m.group(3), a, m.group(2)))
    return items


def candidates(done):
    for f in sorted(glob.glob(os.path.join(ASM, "*_text.s"))):
        obj = os.path.basename(f)[:-2]
        t = open(f, encoding="utf-8", errors="replace").read()
        for m in re.finditer(r"^\.fn (\S+), \w+\n(.*?)^\.endfn", t, re.M | re.S):
            name, body = m.group(1), m.group(2)
            items = parse(body)
            ins = [it for it in items if it[0] == "ins"]
            if not ins or len(ins) > MAXN:
                continue
            addr = int(ins[0][3], 16)
            if not GAME_START <= addr < GAME_END or ins[0][3] in done:
                continue
            yield obj, ins[0][3], name, items


def main():
    done = done_addrs()
    sigs = header_sigs()
    hnames = header_names()
    funcs = func_symbols()
    FUNCS.update(funcs)
    cands = list(candidates(done))
    base_protos = source_protos(funcs)
    for n, (ret, params) in sigs.items():
        pr = norm_proto(ret, params)
        if pr and n not in base_protos:
            base_protos[n] = pr

    heads = {}
    for _ in range(4):      # later passes see the heads of functions generated earlier
        protos = dict(heads)
        protos.update(base_protos)
        total, skipped, files, new_heads = 0, {}, {}, {}
        for obj, addr, name, items in cands:
            try:
                code, globs, calls = gen_function(name, items, sigs.get(name), name in sigs,
                                                  sigs, protos, hnames)
            except Skip as e:
                k = str(e).split()[0]
                skipped[k] = skipped.get(k, 0) + 1
                continue
            key = obj + ("_cb" if name in sigs else "")
            files.setdefault(key, []).append((addr, name, code, globs, calls))
            hp = norm_proto(*sigs[name]) if name in sigs else head_proto(code[0][:-2])
            if hp:
                new_heads[name] = hp
            total += 1
        if new_heads == heads:
            break
        heads = new_heads

    # a target defined in the same file must match the prototype the caller assumed
    dropped = 0
    for key, fns in files.items():
        while True:
            heads_here = {n: (norm_proto(*sigs[n]) if n in sigs else head_proto(c[0][:-2])) for _, n, c, _, _ in fns}
            bad, seen = [], {}
            for f in fns:
                for t, d in f[4].items():
                    if d is None:
                        continue
                    want = heads_here.get(t) or seen.setdefault(t, d)
                    if d != want:
                        bad.append(f)
                        break
            if not bad:
                break
            fns[:] = [f for f in fns if f not in bad]
            dropped += len(bad)
    total -= dropped
    files = {k: v for k, v in files.items() if v}

    print(f"generated {total} functions in {len(files)} files ({dropped} dropped: prototype mismatch)")
    print("skipped:", dict(sorted(skipped.items(), key=lambda x: -x[1])[:14]))
    if DRY:
        return
    os.makedirs(OUT, exist_ok=True)
    for old in glob.glob(os.path.join(OUT, "*.cpp")):
        os.remove(old)
    for key, fns in files.items():
        cb = key.endswith("_cb")
        defined = {n for _, n, _, _, _ in fns}
        lines = ["// Generated by tools/gen_leaf.py: small leaf functions, translated instruction by",
                 "// instruction. Do not edit by hand; decompile into a real source file instead and",
                 "// the generator will leave the function out on the next run.",
                 "#include <stdint.h>", "", '#include "types.h"']
        if cb:
            lines.append('#include "game/script_callbacks.h"')
        lines.append("")
        globs = sorted(set().union(*[g for _, _, _, g, _ in fns]))
        gtypes = global_types()
        calls = {}
        for _, _, _, _, c in fns:
            for t, d in c.items():
                if d is not None and t not in defined:
                    calls.setdefault(t, d)
        heads = {n: c[0][:-2] for _, n, c, _, _ in fns}
        called = {t for _, _, _, _, c in fns for t, d in c.items() if d is not None}
        for n in sorted(called & defined):     # forward declarations for tail calls in this file
            if not (cb and n in hnames):
                lines.append(heads[n] + ";")
        for gname in globs:
            if gname in defined:
                if gname in funcs and not (cb and gname in hnames) and gname not in called:
                    lines.append(heads[gname] + ";")
                continue
            if cb and gname in hnames:
                continue
            if gname in calls:
                continue
            if gname in funcs:
                lines.append(f'extern "C" void {gname}(void);')
            else:
                lines.append(f'extern "C" {gtypes.get(gname, "u8")} {gname}{"" if gname in gtypes else "[]"};')
        for t in sorted(calls):
            ret, ps = calls[t]
            lines.append(f'extern "C" {ret} {t}({", ".join(ps) or "void"});')
        if globs or calls:
            lines.append("")
        for addr, name, code, _, _ in sorted(fns):
            lines.append(f"// 0x{addr}")
            lines += code
            lines.append("")
        src = "\n".join(lines)
        # function symbols are referenced by address
        for gname in globs:
            if gname in funcs:
                src = src.replace(f"(u8*)&{gname}", f"(u8*)(uintptr_t)&{gname}")
        open(os.path.join(OUT, key + ".cpp"), "w", encoding="utf-8").write(src)


if __name__ == "__main__":
    main()
