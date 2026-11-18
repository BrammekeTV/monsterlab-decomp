#!/usr/bin/env python3
"""Draft a batch of undecompiled functions with m2c, clean the output into repo-style C++,
compile it, and report which functions compile as-is and which need fixing.

    python tools/draft_batch.py --size 0x80 0x300 --region 0x8019 --count 12 --name draft_8019
    python tools/draft_batch.py --names fn_801B00EC UIElement_SetRot --name draft_x

Output (never edit by hand what is generated; edit the staging copy):
  build/drafts/<name>.cpp        every drafted function (prelude + prototypes + bodies), compiled
  build/drafts/<name>.report.txt per-function status: OK (compiles), FIX (compile errors or
                                 m2c leftovers), ASM (tail call / jump table / m2c failure:
                                 write it from the asm by hand)
Functions that compile as-is are NOT verified: a draft is only a starting point. Every function
still has to be checked against `sh tools/dump.sh <name>` (tools/draft_fix_brief.md) and then
moved into src/. Use --dest src/game/<name>.cpp to also write the verified-later file directly.
"""
import argparse, concurrent.futures as cf, glob, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import decomp  # noqa: E402

OUT = os.path.join(ROOT, "build", "drafts")
SYMBOLS_TEXT = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")
ADDR_LO, ADDR_HI = 0x80006B40, 0x8022FEC4

PRELUDE = '''#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

// Raw field access helpers for drafts: FLD(type, base, byteOffset).
#define FLD(T, p, off) (*(T*)((u8*)(p) + (off)))
// Virtual / indirect call through the function pointer stored at slotBase + off (draft helper:
// rewrite into the repo's `(*(Fn*)(*(u8**)obj + OFF))(obj, ...)` style when finalizing).
template <class R, class... A> static inline R vcall(void* slotBase, int off, A... a) {
    return ((R (*)(A...)) * (void**)((u8*)slotBase + off))(a...);
}
'''
KEYWORDS = {"if", "while", "for", "switch", "return", "sizeof", "case", "else", "do", "FLD", "defined"}


# ---------------------------------------------------------------- candidate selection
def done_addrs():
    done = set()
    for f in glob.glob(os.path.join(ROOT, "src", "**", "*.c*"), recursive=True):
        txt = open(f, encoding="utf-8", errors="replace").read()
        for m in re.finditer(r"^\s*//\s*0x(80[0-9A-Fa-f]{6})\s*$", txt, re.M):
            done.add(int(m.group(1), 16))
    return done


def candidates(lo, hi, region):
    done = done_addrs()
    out = []
    for l in open(SYMBOLS_TEXT, encoding="utf-8"):
        m = re.match(r"(\S+) = \.text:0x([0-9A-F]{8}); // type:function size:0x([0-9A-F]+)", l)
        if not m:
            continue
        a, s = int(m.group(2), 16), int(m.group(3), 16)
        if ADDR_LO <= a < ADDR_HI and a not in done and lo <= s <= hi and (region is None or a >> 16 == region):
            out.append((a, m.group(1), s))
    return sorted(out)


# ---------------------------------------------------------------- declaration index
_DECL = re.compile(r'^[ \t]*(?:extern "C"[ \t]+)?([A-Za-z_][\w \t\*]*?[ \t\*])(\w+)\(([^;{}]*)\)\s*;', re.M)
_GLOB = re.compile(r'^[ \t]*extern[ \t]+(?:"C"[ \t]+)?([A-Za-z_][\w \t\*]*?[ \t\*])(\w+)(\[[^\]]*\])?\s*;', re.M)


def build_index():
    funcs, globs = {}, {}
    files = sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)) + \
        sorted(glob.glob(os.path.join(ROOT, "include", "**", "*.h"), recursive=True))
    files.sort(key=lambda p: (os.sep + "leaf" + os.sep in p))   # real prototypes before generated leaf ones
    for p in files:
        txt = re.sub(r"/\*.*?\*/", "", open(p, encoding="utf-8", errors="replace").read(), flags=re.S)
        txt = re.sub(r"//[^\n]*", "", txt)
        for m in _DECL.finditer(txt):
            ret, name = m.group(1).strip(), m.group(2)
            if ret.split()[0] in KEYWORDS or "=" in ret:
                continue
            funcs.setdefault(name, f"{ret} {name}({m.group(3).strip()});")
        for m in _GLOB.finditer(txt):
            globs.setdefault(m.group(2), f"extern {m.group(1).strip()} {m.group(2)}{m.group(3) or ''};")
    return funcs, globs


def header_sigs():
    """Script callbacks: name -> (ret, [(type, paramname)])."""
    sigs = {}
    for line in open(os.path.join(ROOT, "include", "game", "script_callbacks.h"), encoding="utf-8"):
        m = re.match(r"^([\w\*\s/]+?)\s+\**(\w+)\(([^)]*)\);", line)
        if not m or line.startswith("//"):
            continue
        params = [] if m.group(3).strip() in ("", "void") else [p.strip() for p in m.group(3).split(",")]
        sigs[m.group(2)] = (m.group(1).strip(), params)
    return sigs


# ---------------------------------------------------------------- m2c + post-processing
def run_m2c(addr, ctx, data, by_addr):
    names = {f"fn_{addr:08X}", by_addr.get(f"{addr:08X}", "")} - {""}
    asm, fn = decomp.find_asm(f"{addr:08X}", names)
    if asm is None:
        return None, "asm not found", ""
    text = open(asm, encoding="utf-8", errors="replace").read()
    m = re.search(rf"^\.fn {re.escape(fn)},.*?^\.endfn {re.escape(fn)}", text, re.S | re.M)
    body = m.group(0) if m else ""
    # Tail calls (branch to another function) and jump tables break m2c.
    if re.search(r"^\s*(?:/\*.*?\*/\s*)?b\s+(?!\.L_)\w+\s*$", body, re.M):
        return None, "tail call", fn
    if re.search(r"\bbctr\b", body):
        return None, "jump table", fn
    r = subprocess.run([sys.executable, decomp.M2C, "-t", "ppc-mwcc-c", "--context", ctx, "-f", fn, asm, *data],
                       capture_output=True, text=True)
    if r.returncode != 0 or not r.stdout.strip():
        return None, "m2c failed: " + (r.stderr.strip().splitlines() or ["?"])[-1][:100], fn
    return r.stdout, None, fn


def rename_syms(src, by_addr):
    return re.sub(r"\b(fn|lbl)_([0-9A-Fa-f]{8})\b", lambda m: by_addr.get(m.group(2).upper(), m.group(0)), src)


def split_m2c(src):
    """-> (declaration lines, function text). m2c puts externs first, then the function."""
    lines = src.splitlines()
    for i, l in enumerate(lines):
        if re.match(r"^[\w\*\s?]+\([^;]*\)\s*\{\s*$", l) and not l.startswith("static"):
            return lines[:i], "\n".join(lines[i:])
    return lines, ""


C_TYPES = {"s8", "s16", "s32", "s64", "u8", "u16", "u32", "u64", "f32", "f64", "BOOL", "void", "char", "int"}


def clean_type(t):
    t = t.strip()
    t = re.sub(r"^\?(\s*\*)?", lambda m: "s32" + (" *" if m.group(1) else ""), t)
    return t.replace("? *", "s32 *")


def convert_fields(body, vartypes):
    """arg->unkN  ->  FLD(T, arg, 0xN); pointer hops become FLD(u8*, ...); *(p + i) -> FLD(T, p, i);
    p->unkS(args) -> vcall<s32>(p, 0xS, args)."""
    body = re.sub(r"\bvoid\s*\*", "u8*", body)
    # *(ptr + byteIndex)
    body = re.sub(r"\*\(((?:\w|->unk[0-9A-Fa-f]+)+) \+ (\w+)\)", r"FLD(@T@, \1, \2)", body)
    ident = r"(?:\b[A-Za-z_]\w*|FLD\((?:[^()]|\([^()]*\))*\))"
    pat = re.compile(r"(" + ident + r")->unk([0-9A-Fa-f]+)")
    for _ in range(12):
        def sub(m):
            tail = body[m.end():m.end() + 6]
            prefix = body[max(0, m.start() - 24):m.start()]
            as_base = re.search(r"FLD\([^,()]+, $", prefix) is not None
            if tail.startswith("("):
                return m.group(0)          # indirect call, handled below
            nxt_ptr = tail.startswith("->unk") or as_base
            return f"FLD({'u8*' if nxt_ptr else '@T@'}, {m.group(1)}, 0x{m.group(2).upper()})"
        new = pat.sub(sub, body)
        if new == body:
            break
        body = new
    # indirect calls: X->unkS(args)  ->  vcall<s32>(X, 0xS, args)
    call = re.compile(r"(" + ident + r")->unk([0-9A-Fa-f]+)\(")
    body = call.sub(lambda m: f"vcall<s32>({m.group(1)}, 0x{m.group(2).upper()}, ", body)
    body = re.sub(r"(vcall<s32>\([^;]*?), (0x[0-9A-F]+), \)", r"\1, \2)", body)

    def typ(v):
        return vartypes.get(v, "s32")
    lines = []
    for l in body.splitlines():
        m = re.match(r"^(\s*)([^=;]+?)\s=\s([^;]+);(.*)$", l)
        if m and "@T@" in l and "==" not in m.group(2):
            lhs, rhs = m.group(2), m.group(3)
            if "@T@" in lhs and re.fullmatch(r"\w+", rhs.strip()):
                lhs = lhs.replace("@T@", typ(rhs.strip()))
            if "@T@" in rhs and re.fullmatch(r"\w+", lhs.strip()):
                rhs = rhs.replace("@T@", typ(lhs.strip()))
            l = f"{m.group(1)}{lhs} = {rhs};{m.group(4)}"
        lines.append(l)
    return chr(10).join(lines).replace("@T@", "s32")


VARTYPES = {}


def call_sites(code, name):
    out = []
    for m in re.finditer(r'\b' + name + r'\(', code):
        i, depth, cur, args = m.end(), 1, '', []
        while i < len(code) and depth:
            c = code[i]
            depth += c == '('
            depth -= c == ')'
            if depth == 0:
                break
            if c == ',' and depth == 1:
                args.append(cur.strip()); cur = ''
            else:
                cur += c
            i += 1
        if cur.strip():
            args.append(cur.strip())
        out.append(args)
    return out


def arg_type(e, vt):
    e = e.strip()
    if e.startswith('"'):
        return 'const char*'
    if e.startswith('&'):
        t = vt.get(e[1:].strip())
        return t + '*' if t and not t.endswith('*') else 'u8*'
    v = vt.get(e)
    if v:
        if v.endswith('*'):
            return 'u8*'
        if v in ('f32', 'f64'):
            return v
    if re.fullmatch(r'-?[0-9]*[.][0-9]+f?', e) or e.startswith('FLD(f32'):
            return 'f32'
    if e.startswith('FLD(u8*') or re.match(r'^(\w+) [+] 0x', e) and (vt.get(e.split()[0], '').endswith('*')):
        return 'u8*'
    return 's32'


def process_function(fn_text, hsigs, name):
    """Clean one m2c function. Returns (code, notes)."""
    notes = []
    head, _, rest = fn_text.partition("{\n")
    mh = re.match(r"^(.*?)\b(\w+)\((.*)\)\s*$", head.strip())
    ret, fname, params = clean_type(mh.group(1)), mh.group(2), mh.group(3)
    plist = [] if params.strip() in ("", "void") else [clean_type(p) for p in params.split(",")]
    body = rest.rsplit("}", 1)[0]
    # parameter types: dereferenced / offset arguments are pointers
    names = []
    for i, p in enumerate(plist):
        pm = re.match(r"^(.*?)(\w+)$", p.strip())
        t, n = pm.group(1).strip(), pm.group(2)
        if t in ("s32", "u32") and (re.search(rf"\b{n}->unk", body) or re.search(rf"\b{n}\s*\+\s*0x", body)):
            t = "u8*"
        t = re.sub(r"void\s*\*", "u8*", t)
        names.append((t, n))
    if fname in hsigs:                                          # script callback: header signature wins
        hret, hparams = hsigs[fname]
        ret = hret
        newp = []
        for i, hp in enumerate(hparams):
            pm = re.match(r"^(.*?)(\w+)$", hp)
            newp.append((pm.group(1).strip() or hp, pm.group(2)))
        for (t_old, n_old), (t_new, n_new) in zip(names, newp):
            body = re.sub(rf"\b{n_old}\b", n_new, body)
        names = newp
    vartypes = {}
    for m in re.finditer(r"^\s*(\w+(?:\s*\*+)?)\s*(\w+);", body, re.M):
        vartypes[m.group(2)] = m.group(1).replace(" ", "").replace("void*", "u8*")
    for t, n in names:
        vartypes[n] = t
    body = re.sub(r"^(\s*)\?(\s*\*)?\s*(\w+);", lambda m: f"{m.group(1)}{'const char*' if m.group(2) else 's32'} {m.group(3)};", body, flags=re.M)
    VARTYPES[fname] = vartypes
    body = convert_fields(body, vartypes)
    body = re.sub(r"\bM2C_\w+\b", lambda m: m.group(0), body)
    if re.search(r"M2C_|saved_reg|MIPS2C|\?\?\?|/\* irregular \*/", body):
        notes.append("m2c leftovers")
    sig = ", ".join(f"{t} {n}" for t, n in names) or "void"
    return f"{ret} {fname}({sig}) {{\n{body.strip(chr(10))}\n}}\n", notes, fname, ret, names


# ---------------------------------------------------------------- main
def compile_file(path):
    bat = None
    import check as chk
    bat = chk.vcvars()
    with tempfile.TemporaryDirectory() as out:
        script = os.path.join(out, "run.bat")
        open(script, "w").write("@call \"" + bat + "\" >nul\r\n@cl " + " ".join(chk.FLAGS) + " /Fo\"" + out + "\\\\\" \"" + path + "\"\r\n")
        r = subprocess.run(["cmd", "/c", script], capture_output=True, text=True)
    errs = []
    for l in (r.stdout + r.stderr).splitlines():
        m = re.match(r"^.*?\((\d+)\): (?:fatal )?error (C\d+): (.*)$", l)
        if m:
            errs.append((int(m.group(1)), m.group(2), m.group(3)))
    return errs


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--size", nargs=2, default=["0x80", "0x300"])
    ap.add_argument("--region", default=None, help="64 KB region, e.g. 0x8019")
    ap.add_argument("--count", type=int, default=15)
    ap.add_argument("--names", nargs="*", default=None)
    ap.add_argument("--name", default=None, help="draft file basename, e.g. draft_8019")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--dest", default=None, help="also copy the draft to this path (e.g. src/game/draft_8019.cpp)")
    a = ap.parse_args()
    by_addr, by_name = decomp.load_symbols()
    if a.names:
        picks = []
        for n in a.names:
            ad = int(n[3:], 16) if n.startswith("fn_") else int(by_name[n], 16)
            picks.append((ad, n, 0))
    else:
        region = int(a.region, 0) if a.region else None
        if region is not None and region > 0xFFFF:
            region >>= 16
        picks = candidates(int(a.size[0], 0), int(a.size[1], 0), region)[:a.count]
    if not picks:
        sys.exit("no candidates")
    name = a.name or f"draft_{picks[0][0] >> 16:04X}"
    os.makedirs(OUT, exist_ok=True)
    ctx = decomp.build_context()
    data = sorted({p for g in decomp.DATA for p in glob.glob(os.path.join(decomp.ASM, g))})
    funcs_idx, globs_idx = build_index()
    hsigs = header_sigs()

    with cf.ThreadPoolExecutor(a.jobs) as ex:
        results = list(ex.map(lambda p: run_m2c(p[0], ctx, data, by_addr), picks))

    chunks, report, prototypes, globals_decl = [], [], {}, {}
    asm_only = []
    for (addr, nm, size), (out, err, fn) in zip(picks, results):
        label = by_addr.get(f"{addr:08X}", f"fn_{addr:08X}")
        if err:
            report.append((label, addr, "ASM", err))
            asm_only.append(label)
            continue
        out = rename_syms(out, by_addr)
        decls, ftext = split_m2c(out)
        if not ftext:
            report.append((label, addr, "ASM", "no function body in m2c output"))
            continue
        try:
            code, notes, fname, ret, params = process_function(ftext, hsigs, label)
        except Exception as e:                                           # noqa: BLE001
            report.append((label, addr, "ASM", f"unsupported signature/construct ({type(e).__name__}); write from asm"))
            continue
        for d in decls:
            m = re.match(r"^\s*(?:extern\s+)?(.*?)\b(\w+)\(([^)]*)\);", d)
            if m and "/* extern */" in d:
                guess_ret = clean_type(m.group(1)) or "s32"
                ps = [] if m.group(3).strip() in ("", "void") else [clean_type(p) for p in m.group(3).split(",")]
                ps = [("u8*" if p.startswith("void") else p) for p in ps]
                prototypes.setdefault(m.group(2), (guess_ret, ps))
            else:
                g = re.match(r"^\s*(?:extern|static)?\s*([\w\*\s\?]+?)\s+(\w+)(\[[^\]]*\])?;", d)
                if g:
                    globals_decl.setdefault(g.group(2), (clean_type(g.group(1)), g.group(3) or ""))
        chunks.append((label, addr, code, notes, fname, ret, params))

    # prototypes for callees
    own = {c[4] for c in chunks}
    called = set()
    for c in chunks:
        called |= set(re.findall(r"\b([A-Za-z_]\w*)\(", c[2])) - KEYWORDS - own
    proto_lines = []
    for n in sorted(called):
        if n in hsigs:
            continue
        if n in funcs_idx:
            proto_lines.append(funcs_idx[n])
        elif n in prototypes:
            r, ps = prototypes[n]
            for c in chunks:
                sites = call_sites(c[2], n)
                if sites:
                    ps = [arg_type(x, VARTYPES.get(c[4], {})) for x in sites[0]]
                    break
            addr = decomp.load_symbols()[1].get(n, "")
            proto_lines.append(f"{r} {n}({', '.join(ps) or 'void'});  // 0x{addr}, signature guessed by draft_batch" if addr else f"{r} {n}({', '.join(ps) or 'void'});")
    glob_lines = []
    used = set(re.findall(r"\b(lbl_\w+|g[A-Z]\w+)\b", "\n".join(c[2] for c in chunks)))
    for n in sorted(used):
        if n in globs_idx:
            glob_lines.append(globs_idx[n])
        elif n in globals_decl:
            t, arr = globals_decl[n]
            addr = re.match(r"lbl_([0-9A-F]{8})", n)
            glob_lines.append(f"extern {t if t != '?' else 'u8'} {n}{arr};" + (f"  // 0x{addr.group(1)}" if addr else ""))

    unk = {n for n, (t, arr) in globals_decl.items() if (t == '?' and n not in globs_idx) or arr} | {n for n, d in globs_idx.items() if '[' in d}
    chunks = [(l, a, re.sub(r'&(' + '|'.join(sorted(unk)) + r')\b', r'\1', c) if unk else c, *rest) for (l, a, c, *rest) in chunks]
    head = PRELUDE + '\nextern "C" {\n' + "\n".join(proto_lines + glob_lines) + "\n}\n\nextern \"C\" {\n\n"
    body = ""
    for label, addr, code, notes, fname, ret, params in chunks:
        body += f"// 0x{addr:08X}\n{code}\n"
    path = os.path.join(OUT, name + ".cpp")
    full = head + body + '}  // extern "C"\n'
    open(path, "w", encoding="utf-8").write(full)

    # compile and attribute errors to functions
    errs = compile_file(path)
    starts = []
    for label, addr, *_ in chunks:
        starts.append((full[:full.index(f"// 0x{addr:08X}\n")].count("\n") + 1, label))
    bad = {}
    for ln, code, msg in errs:
        owner = None
        for s, label in starts:
            if ln >= s:
                owner = label
        bad.setdefault(owner or "<prelude>", []).append(f"line {ln} {code} {msg[:90]}")
    for label, addr, code, notes, *_ in chunks:
        if label in bad:
            report.append((label, addr, "FIX", f"{len(bad[label])} compile error(s); first: {bad[label][0]}"))
        elif notes:
            report.append((label, addr, "FIX", ", ".join(notes)))
        else:
            report.append((label, addr, "OK", "compiles (unverified against asm)"))
    if "<prelude>" in bad:
        report.append(("<prelude>", 0, "FIX", "; ".join(bad["<prelude>"][:3])))
    rp = os.path.join(OUT, name + ".report.txt")
    lines = [f"{st:4} 0x{ad:08X} {lb}: {why}" for lb, ad, st, why in sorted(report, key=lambda r: r[1])]
    open(rp, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("\n".join(lines))
    c = {s: sum(1 for r in report if r[2] == s) for s in ("OK", "FIX", "ASM")}
    print(f"\n{path}\n{rp}\nOK {c['OK']}  FIX {c['FIX']}  ASM {c['ASM']}")
    if a.dest:
        open(os.path.join(ROOT, a.dest), "w", encoding="utf-8", newline="").write(full)
    return 0


if __name__ == "__main__":
    sys.exit(main())
