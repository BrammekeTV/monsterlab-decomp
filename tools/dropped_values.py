#!/usr/bin/env python3
"""Find values the original code passes or returns that the C dropped.

    python tools/dropped_values.py [param zero const ret] [--boot] [--files SUBSTR ...] [--all]

Reads the asm (build/RM7P4F/asm, from `dtk dol split`) and the `// 0xADDR` definitions in src/.
First works out which argument registers every function really reads on entry (through calls),
then reports:

  param  the asm reads an argument register that the C definition has no parameter for
  zero   the C passes a literal 0/NULL where the asm does not load 0 into that register
         although the callee reads it
  const  the C passes another literal than the constant the asm loads
  ret    the asm uses r3/f1 after `bl X`, but X is void in C, or the C caller drops the result

Every line is a candidate, not a verdict: check it with `sh tools/dump.sh <name>`.
--boot limits the report to files named in docs/boot_path_functions.txt plus battle, village and
Field Repair files; --files limits it to paths containing one of the substrings. Without --all,
hits listed in tools/dropped_values_ok.txt (checked, not a bug) are left out.
"""
import glob, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
ASM = os.path.join(ROOT, "build", "RM7P4F", "asm")
OK_FILE = os.path.join(HERE, "dropped_values_ok.txt")

GPR_ARGS = ["r%d" % i for i in range(3, 11)]
FPR_ARGS = ["f%d" % i for i in range(1, 9)]
ARG_REGS = GPR_ARGS + FPR_ARGS
VOLATILE = {"r0"} | {"r%d" % i for i in range(3, 13)} | {"f%d" % i for i in range(0, 14)}

REG = re.compile(r"(?<![\w@.])([rf](?:[12]?\d|3[01]))(?![\w@])")


# ------------------------------------------------------------------ asm
class Ins:
    __slots__ = ("op", "args", "uses", "defs", "addr")

    def __init__(self, addr, op, args):
        self.addr, self.op, self.args = addr, op, args
        self.uses, self.defs = effects(op, args)


def effects(op, args):
    """(registers read, registers written) of one non-call instruction."""
    if op.startswith("b"):
        return set(), set()
    parts = [p.strip() for p in args.split(",")]
    regs = [REG.findall(p) for p in parts]
    allr = [r for rs in regs for r in rs]
    if op.startswith(("st", "psq_st", "cmp", "fcmp", "ps_cmp", "mt", "dcb", "icb", "tw")):
        d = set()
        if op.endswith(("u", "ux")) and op.startswith(("st", "psq_st")) and len(regs) > 1:
            d = set(regs[1][-1:])
        return set(allr), d
    if not parts or not regs[0] or op.startswith("cr"):
        return set(allr), set()
    defs = set(regs[0])
    uses = {r for rs in regs[1:] for r in rs}
    if op.startswith(("rlwimi", "insl", "insr")):
        uses |= defs
    if op.startswith(("l", "psq_l")) and op.rstrip(".").endswith(("u", "ux")) and op != "lbzu_" and len(regs) > 1:
        defs |= set(regs[1][-1:])
    return uses, defs


class Func:
    def __init__(self, name, addr):
        self.name, self.addr = name, addr
        self.ins = []
        self.labels = {}
        self.variadic = False
        self.jumps = {}        # jump table name -> instruction indices it reaches

    def succ(self, i):
        x = self.ins[i]
        op, n = x.op.rstrip("+-"), len(self.ins)
        nxt = [i + 1] if i + 1 < n else []
        if op == "blr":
            return []
        if op == "bctr":
            # the table this dispatch loads (none: an indirect tail call)
            for k in range(i - 1, max(-1, i - 10), -1):
                m = re.search(r"(jumptable_\w+)", self.ins[k].args)
                if m:
                    return self.jumps.get(m.group(1), [])
            return []
        if op in ("bl", "bctrl", "blrl") or not op.startswith("b") or op.endswith("lr"):
            return nxt
        tgt = x.args.split(",")[-1].strip()
        j = self.labels.get(tgt)
        out = [] if j is None or j >= n else [j]
        if op == "b":
            return out
        return out + nxt


def call_target(f, x):
    """Callee name for `bl X` and tail calls `b X`, else None."""
    op = x.op.rstrip("+-")
    if op == "bl":
        return x.args.strip()
    if op == "b" and not x.args.startswith(".L_"):
        return x.args.strip()
    return None


def load_asm():
    funcs, by_name, floats, tables = {}, {}, {}, []
    for path in glob.glob(os.path.join(ASM, "*.s")):
        cur, obj, pend = None, None, []
        for line in open(path, encoding="utf-8", errors="replace"):
            if line.startswith("/* ") and cur is not None:
                m = re.match(r"/\* ([0-9A-F]{8}) [0-9A-F]{8}  [0-9A-F ]{11} \*/\t(\S+)\s*(.*)", line.rstrip())
                if m:
                    if cur.addr is None:
                        cur.addr = int(m.group(1), 16)
                    for l in pend:
                        cur.labels[l] = len(cur.ins)
                    pend = []
                    if m.group(2) == "bne" and m.group(3).startswith("cr1,"):
                        cur.variadic = True
                    cur.ins.append(Ins(int(m.group(1), 16), m.group(2), m.group(3)))
            elif line.startswith(".fn "):
                cur = Func(line[4:].split(",")[0].strip().strip('"'), None)
            elif line.startswith(".endfn"):
                if cur and cur.addr is not None:
                    funcs[cur.addr] = cur
                    by_name.setdefault(cur.name, cur.addr)
                cur = None
            elif line.startswith(".L_") and cur is not None:
                pend.append(line.strip().rstrip(":"))
            elif line.startswith(".obj "):
                obj = line[5:].split(",")[0].strip().strip('"')
            elif obj and line.startswith(("\t.float ", "\t.double ")):
                floats.setdefault(obj, line.split()[1])
                obj = None
            elif line.startswith("\t.4byte ") and obj and obj.startswith("jumptable"):
                m = re.match(r"\t\.4byte (\S+?)\+0x([0-9A-Fa-f]+)", line)
                if m:
                    tables.append((obj, m.group(1), int(m.group(2), 16)))
            elif line.startswith(".endobj"):
                obj = None
    for table, name, off in tables:
        f = funcs.get(by_name.get(name.strip('"')))
        if f and off // 4 < len(f.ins):
            f.jumps.setdefault(table, []).append(off // 4)
    return funcs, by_name, floats


# ------------------------------------------------------------------ live-in analysis
def helper(name):
    return name.startswith(("_savegpr", "_restgpr", "_savefpr", "_restfpr", "__save", "__rest"))


def live_in(f, callee_in):
    """Argument registers read before being written on some path from the entry."""
    n = len(f.ins)
    live = [set() for _ in range(n + 1)]
    succ = [f.succ(i) for i in range(n)]
    changed = True
    while changed:
        changed = False
        for i in range(n - 1, -1, -1):
            x = f.ins[i]
            out = set()
            for j in succ[i]:
                out |= live[j]
            t = call_target(f, x)
            op = x.op.rstrip("+-")
            if t is not None and not helper(t):
                new = callee_in(t) | (out - VOLATILE) if op == "bl" else callee_in(t) | out
            elif op in ("bctrl", "blrl"):
                new = out - VOLATILE
            else:
                new = (out - x.defs) | x.uses
            if new != live[i]:
                live[i] = new
                changed = True
    return live[0] if n else set()


def all_live_in(funcs, by_name):
    res = {a: set() for a in funcs}

    def callee_in(name):
        a = by_name.get(name)
        return res.get(a, set()) if a is not None else set()

    for _ in range(12):
        changed = False
        for a, f in funcs.items():
            if f.variadic:
                continue
            v = live_in(f, callee_in) & set(ARG_REGS)
            # a saved argument register that is only spilled for a va_list is not evidence
            if v != res[a]:
                res[a] = v
                changed = True
        if not changed:
            break
    return res


# ------------------------------------------------------------------ values at call sites
def forward(f, floats):
    """For every call index: register -> description of what it holds ('0', 'c:N', 'entry', ...)."""
    n = len(f.ins)
    start = {r: "entry:" + r for r in ARG_REGS}
    state = [None] * n
    if not n:
        return {}
    state[0] = start
    work = [0]
    calls = {}
    while work:
        i = work.pop()
        x = f.ins[i]
        s = dict(state[i])
        op = x.op.rstrip("+-")
        t = call_target(f, x)
        if t is not None and helper(t):
            pass
        elif t is not None or op in ("bctrl", "blrl", "bctr"):
            calls[i] = dict(s)
            if op != "b":
                for r in VOLATILE:
                    s[r] = "stale"
                s["r3"] = "ret"
                s["f1"] = "ret"
        else:
            p = [a.strip() for a in x.args.split(",")]
            val = None
            if op == "li":
                val = "c:%d" % int(p[1], 0) if re.match(r"-?(0x)?[0-9A-Fa-f]+$", p[1]) else "val"
            elif op in ("mr", "fmr"):
                val = s.get(p[1], "val")
            elif op == "lis" and re.match(r"-?(0x)?[0-9A-Fa-f]+$", p[1]):
                val = "c:%d" % ((int(p[1], 0) << 16) & 0xFFFFFFFF)
            elif op in ("addi", "subi", "ori") and len(p) == 3 and s.get(p[1], "").startswith("c:") and                     re.match(r"-?(0x)?[0-9A-Fa-f]+$", p[2]):
                a, b = int(float(s[p[1]][2:])), int(p[2], 0)
                val = "c:%d" % ((a + b if op == "addi" else a - b if op == "subi" else a | b) & 0xFFFFFFFF
                                if abs(a) > 0xFFFF else (a + b if op == "addi" else a - b if op == "subi" else a | b))
            elif op in ("lfs", "lfd"):
                lab = p[1].split("@")[0]
                if lab in floats:
                    val = "c:%g" % float(floats[lab])
            for d in x.defs:
                s[d] = val if val is not None else "val"
        for j in f.succ(i):
            old = state[j]
            if old is None:
                new = s
            else:
                new = {}
                for r in set(old) | set(s):
                    a, b = old.get(r, "val"), s.get(r, "val")
                    new[r] = a if a == b else ("val" if "stale" not in (a, b) else (a if b == "stale" else b))
            if new != old:
                state[j] = new
                work.append(j)
    return calls


def returns_this(funcs, by_name, a, seen=frozenset()):
    """True if every path of the function returns with r3 as it came in."""
    f = funcs.get(a)
    if not f or a in seen:
        return False
    done, work, ok = set(), [(0, frozenset(["r3"]))], False
    while work:
        i, s = work.pop()
        if (i, s) in done:
            continue
        done.add((i, s))
        x, s = f.ins[i], set(s)
        t, op = call_target(f, x), x.op.rstrip("+-")
        if t is not None and helper(t):
            pass
        elif t is not None or op in ("bctrl", "blrl", "bctr"):
            keep = "r3" in s and t is not None and returns_this(funcs, by_name, by_name.get(t), seen | {a})
            if op == "b":
                if not keep:
                    return False
                ok = True
                continue
            s -= VOLATILE
            if keep:
                s.add("r3")
        elif op == "blr":
            if "r3" not in s:
                return False
            ok = True
            continue
        elif op in ("mr", "fmr"):
            d, src = [p.strip() for p in x.args.split(",")]
            s.discard(d)
            if src in s:
                s.add(d)
        else:
            s -= x.defs
        work.extend((j, frozenset(s)) for j in f.succ(i))
    return ok


def result_used(f, i, reg):
    """True if `reg` is read after the call at index i before being overwritten."""
    seen, work = set(), f.succ(i)
    work = list(work)
    while work:
        j = work.pop()
        if j in seen:
            continue
        seen.add(j)
        x = f.ins[j]
        t = call_target(f, x)
        op = x.op.rstrip("+-")
        if t is not None and not helper(t):
            if reg in LIVEIN.get(BY_NAME.get(t), ()):
                return True
            continue
        if op in ("bctrl", "bctr", "blr", "blrl"):
            continue
        if reg in x.uses:
            return True
        if reg in x.defs:
            continue
        work.extend(f.succ(j))
    return False


# ------------------------------------------------------------------ C side
def strip_code(txt):
    """Blank out comments and strings, keeping offsets and line breaks."""
    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    return re.sub(r"//[^\n]*|/\*.*?\*/|\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*'", blank, txt, flags=re.S)


def split_args(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return [] if out == ["void"] else out


def close_paren(s, i, o="(", c=")"):
    depth = 0
    for k in range(i, len(s)):
        if s[k] == o:
            depth += 1
        elif s[k] == c:
            depth -= 1
            if depth == 0:
                return k
    return len(s) - 1


def param_regs(params):
    """Argument registers of each C parameter, None if it cannot be told."""
    g, fp, out = 3, 1, []
    for p in params:
        if p == "...":
            return None
        ptr = "*" in p or "&" in p or "[" in p
        if not ptr and re.search(r"\b(f32|f64|float|double)\b", p):
            out.append(["f%d" % fp])
            fp += 1
        elif not ptr and re.search(r"\b(s64|u64|long long|OSTime)\b", p):
            g += (g + 1) % 2
            out.append(["r%d" % g, "r%d" % (g + 1)])
            g += 2
        elif not ptr and re.search(r"\b(Vec3f?|Mtx|Color|GXColor)\b", p) and not p.startswith("GXColor"):
            out.append(["r%d" % g])
            g += 1
        else:
            out.append(["r%d" % g])
            g += 1
    return out


HEAD = re.compile(r"^[ \t]*(?:(?:extern \"C\"|static|inline)[ \t]+)*([A-Za-z_][\w \t\*&:<>,]*?[\s\*&])((?:\w+::)?\w+)[ \t]*\(", re.M)


class CDef:
    def __init__(self, path, line, addr, ret, name, params, body, body_off):
        self.path, self.line, self.addr = path, line, addr
        self.ret, self.name, self.params = ret, name, params
        self.body, self.body_off = body, body_off


def load_c():
    defs, names = {}, {}
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True):
        rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
        if rel.startswith("src/port/"):
            continue
        txt = open(path, encoding="utf-8", errors="replace").read()
        code = strip_code(txt)
        for m in re.finditer(r"(\w+)\s*\([^;{}]*\)\s*;\s*//\s*0x(8[0-9A-Fa-f]{7})", txt):
            names.setdefault(m.group(1), int(m.group(2), 16))
        for m in re.finditer(r"^[ \t]*//[ \t]*0x(8[0-9A-Fa-f]{7})[ \t]*$", txt, re.M):
            addr = int(m.group(1), 16)
            pos = m.end()
            while True:
                h = HEAD.search(code, pos)
                if not h or h.start() - m.end() > 3000:
                    h = None
                    break
                p0 = h.end() - 1
                p1 = close_paren(code, p0)
                after = re.match(r"\s*(?:const\s*)?\{", code[p1 + 1:])
                if after or re.match(r"\s*(?:const\s*)?;", code[p1 + 1:]):
                    break          # a definition, or the marker belongs to a declaration
                pos = p1 + 1
            if not h or not after:
                continue
            b0 = p1 + 1 + after.end() - 1
            b1 = close_paren(code, b0, "{", "}")
            name = h.group(2)
            defs[addr] = CDef(rel, txt.count("\n", 0, h.start()) + 1, addr, h.group(1).strip(), name,
                              split_args(code[p0 + 1:p1]), code[b0:b1 + 1], txt.count("\n", 0, b0) + 1)
            names.setdefault(name, addr)
    return defs, names


def load_protos():
    """name -> parameter list of the prototypes in src/ and include/ (for callees not defined here)."""
    out = {}
    for path in glob.glob(os.path.join(ROOT, "include", "**", "*.h"), recursive=True) +             glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True):
        code = strip_code(open(path, encoding="utf-8", errors="replace").read())
        for m in re.finditer(r"^[ \t]*(?:extern \"C\"[ \t]+|extern[ \t]+)?[A-Za-z_][\w \t\*&]*?[ \t\*&](\w+)[ \t]*\(([^;{}]*)\)[ \t]*;", code, re.M):
            params = split_args(m.group(2))
            if all(re.search(r"\b(void|char|int|float|double|bool|[usf]\d+|BOOL|size_t)\b|\*|&|[A-Z]\w+", x) for x in params):
                out.setdefault(m.group(1), params)
    return out


def guess_params(args):
    return ["f32 x" if re.match(r"^(\(f(32|64)\)|-?\d+\.\d*f?$)", x) else "s32 x" for x in args]


ZERO = re.compile(r"^(?:\(\s*(?:const\s+)?[\w ]+\s*\**\s*\)\s*)*\(?(0|0x0|0u|0U|NULL|nullptr|0\.0f?|0\.f|FALSE|false)\)?$")


LITERAL = re.compile(r"^(?:\(\s*(?:const\s+)?[\w ]+\s*\**\s*\)\s*)*(-?(?:0x[0-9A-Fa-f]+|\d+(?:\.\d*)?))[uUfF]?$")


def literal(arg):
    if arg in ("NULL", "nullptr", "FALSE", "false"):
        return 0.0
    if arg in ("TRUE", "true"):
        return 1.0
    m = LITERAL.match(arg)
    if not m:
        return None
    t = m.group(1)
    return float(int(t, 16)) if "0x" in t else float(t)


def same_const(held, v):
    h = float(held[2:])
    return abs(h - v) <= abs(v) * 1e-5 or (h - v) % 4294967296 == 0


def calls_in(body):
    """(name, offset, args, statement-level) for every plain call in a function body."""
    for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", body):
        p0 = m.end() - 1
        p1 = close_paren(body, p0)
        before = body[:m.start()].rstrip()
        stmt = (not before or before[-1] in ";{})" or before.endswith(("else", "(void)", ":"))) and \
            body[p1 + 1:].lstrip().startswith(";")
        if before.endswith(")") and not re.search(r"\b(if|while|for)\s*$", before[:before.rfind("(")]):
            stmt = False
        yield m.group(1), m.start(), split_args(body[p0 + 1:p1]), stmt


# ------------------------------------------------------------------ report
def boot_files():
    out = set()
    p = os.path.join(ROOT, "docs", "boot_path_functions.txt")
    if os.path.exists(p):
        out |= set(re.findall(r"[\w]+\.cpp", open(p, encoding="utf-8", errors="replace").read()))
    return out


GAMEPLAY = re.compile(r"battle|village|repair|minigame|combat|attack|monster|part|limb|lab|quest|zone|world|camera|ui_|gameflow", re.I)

LIVEIN, BY_NAME = {}, {}


def main():
    argv = sys.argv[1:]
    every = ["param", "zero", "const", "ret"]
    kinds = [a for a in argv if a in every] or every
    only_files = argv[argv.index("--files") + 1:] if "--files" in argv else []
    funcs, by_name, floats = load_asm()
    if not funcs:
        print("no asm in build/RM7P4F/asm: run dtk dol split first (see AGENTS.md)")
        return 1
    BY_NAME.update(by_name)
    LIVEIN.update(all_live_in(funcs, by_name))
    defs, cnames = load_c()
    protos = load_protos()
    ok = set()
    if "--all" not in argv and os.path.exists(OK_FILE):
        ok = {tuple(l.split()[:3]) for l in open(OK_FILE, encoding="utf-8") if l.strip() and not l.startswith("#")}
    boot = boot_files() if "--boot" in argv else None

    def addr_of(cname):
        a = cnames.get(cname)
        return a if a is not None else by_name.get(cname)

    def wanted(d):
        if d.path.startswith("src/leaf/"):
            return False
        if only_files:
            return any(s in d.path for s in only_files)
        if boot is not None:
            base = os.path.basename(d.path)
            return base in boot or bool(GAMEPLAY.search(base))
        return True

    hits = []

    def hit(kind, d, line, key, msg):
        if (kind, "%08X" % d.addr, key) not in ok:
            hits.append((kind, d.path, line, "%08X" % d.addr, key, msg))

    void_used = {}
    for a, d in sorted(defs.items()):
        f = funcs.get(a)
        if not f:
            continue
        # -------- param
        if "param" in kinds and wanted(d) and not f.variadic:
            regs = param_regs(d.params)
            if regs is not None and "::" not in d.name:
                have = {r for rs in regs for r in rs}
                if d.ret in ("Vec3", "Vec3f", "Mtx34"):
                    have = None
                if have is not None:
                    miss = sorted(LIVEIN.get(a, set()) - have, key=ARG_REGS.index)
                    if miss:
                        key = "+".join(miss)
                        hit("param", d, d.line, key, "%s(%s): asm reads %s on entry" % (d.name, ", ".join(d.params), key))
        calls = list(calls_in(d.body))
        if not wanted(d) and "ret" not in kinds:
            continue
        states = forward(f, floats)
        sites = {}
        for i in states:
            t = call_target(f, f.ins[i])
            if t is not None:
                ta = by_name.get(t)
                if ta is not None:
                    sites.setdefault(ta, []).append(i)
        csites = {}
        for name, off, args, stmt in calls:
            ta = addr_of(name)
            if ta is not None:
                csites.setdefault(ta, []).append((name, off, args, stmt))
        for ta, lst in csites.items():
            asm_sites = sorted(sites.get(ta, []))
            if not asm_sites:
                continue
            callee = defs.get(ta)
            # -------- zero
            if "zero" in kinds and wanted(d):
                flagged = {}
                for name, off, args, stmt in lst:
                    sig = callee.params if callee else protos.get(name)
                    if sig is None or len(sig) != len(args):
                        sig = guess_params(args)
                    regs = param_regs(sig)
                    if not regs:
                        continue
                    for k, arg in enumerate(args):
                        if not ZERO.match(arg) or k >= len(regs):
                            continue
                        r = regs[k][0]
                        if r not in LIVEIN.get(ta, set()):
                            continue
                        flagged.setdefault((k, r, name), []).append(off)
                for (k, r, name), offs in flagged.items():
                    zeros = sum(1 for i in asm_sites if states[i].get(r) == "c:0")
                    if zeros >= len(offs) or zeros == len(asm_sites):
                        continue
                    seen = ",".join(sorted({states[i].get(r, "val") for i in asm_sites}))
                    line = d.body_off + d.body.count("\n", 0, offs[0])
                    hit("zero", d, line, "%s:%s" % (name, r),
                        "%s -> %s arg %d (%s): C passes 0 (%d site(s)), asm holds %s" % (d.name, name, k + 1, r, len(offs), seen))
            # -------- const: a non-zero literal where every asm call loads another constant
            if "const" in kinds and wanted(d):
                for name, off, args, stmt in lst:
                    sig = callee.params if callee else protos.get(name)
                    if sig is None or len(sig) != len(args):
                        sig = guess_params(args)
                    regs = param_regs(sig)
                    if not regs:
                        continue
                    for k, arg in enumerate(args):
                        v = literal(arg)
                        if v is None or ZERO.match(arg) or k >= len(regs):
                            continue
                        r = regs[k][0]
                        if r not in LIVEIN.get(ta, set()):
                            continue
                        asm_vals = [states[i].get(r, "val") for i in asm_sites]
                        c_vals = [literal(c[2][k]) if c[0] == name and k < len(c[2]) else None for c in lst]
                        if len(lst) == len(asm_sites) and None not in c_vals and all(h.startswith("c:") for h in asm_vals):
                            # one asm call per C call: the same constants must appear, in any order
                            left = list(asm_vals)
                            for cv in c_vals:
                                m = next((h for h in left if same_const(h, cv)), None)
                                if m:
                                    left.remove(m)
                                elif cv == v:
                                    break
                            else:
                                continue
                            held = set(left)
                        else:
                            held = set(asm_vals)
                        if all(h.startswith("c:") and not same_const(h, v) for h in held):
                            line = d.body_off + d.body.count("\n", 0, off)
                            hit("const", d, line, "%s:%s" % (name, r), "%s -> %s arg %d (%s): C passes %s, asm holds %s" %
                                (d.name, name, k + 1, r, arg, ",".join(sorted(held))))
            # -------- ret
            if "ret" in kinds:
                for reg in ("r3", "f1"):
                    used = [i for i in asm_sites if f.ins[i].op.rstrip("+-") == "bl" and result_used(f, i, reg)]
                    if not used or not callee:
                        continue
                    if reg == "r3" and returns_this(funcs, by_name, ta):
                        continue
                    if callee.ret == "void":
                        void_used.setdefault((ta, reg), []).append(d)
                    else:
                        is_float = callee.ret in ("f32", "f64", "float", "double")
                        if (reg == "f1") != is_float:
                            continue
                        dropped = [c for c in lst if c[3]]
                        if dropped and len(used) > len(lst) - len(dropped) and wanted(d):
                            line = d.body_off + d.body.count("\n", 0, dropped[0][1])
                            hit("ret", d, line, "%s:%s" % (dropped[0][0], reg),
                                "%s ignores %s's result, asm reads %s after the call" % (d.name, dropped[0][0], reg))
    for (ta, reg), users in sorted(void_used.items()):
        c = defs[ta]
        if wanted(c) or any(wanted(u) for u in users):
            hit("ret", c, c.line, "void:%s" % reg, "%s is void, but %s read(s) %s after calling it" %
                (c.name, ", ".join(sorted({u.name for u in users})[:4]), reg))

    for kind, path, line, addr, key, msg in sorted(hits):
        print("%-5s %s:%d [%s %s] %s" % (kind, path, line, addr, key, msg))
    print("dropped_values: %d candidate(s) %s" % (len(hits), {k: sum(h[0] == k for h in hits) for k in kinds}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
