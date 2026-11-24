#!/usr/bin/env python3
"""Lint decompiled sources for the mistakes helpers keep making. Findings are file:line.

    python tools/lint_decomp.py [files...]     (default: files changed since the last commit)
    python tools/lint_decomp.py --all          (every file under src/ except src/leaf)

Checks
  marker   `// 0xADDR` must be alone on its line (progress.py / small_candidates.py only count those)
  markname the first code after a `// 0xADDR` marker is a declaration (prototype, struct, typedef) of
           another name: tools/port_link.py binds the address to the first name after a marker, so calls
           to that Wii address would run the declared function (put declarations above the marker)
  ptrcast  pointer-truncating casts: (u32)(uintptr_t)..., (BOOL)ptr, (u32)&sym, (s32)f() where f returns
           a pointer, `*(u32*)(..) = &sym / (u8*)x` stores of addresses (the PC build is x64)
  tail     the asm ends with a tail call `b X`, but the C drops X's return value (void function, or
           a plain `X(...);` as the last statement while the function returns a value)
  passthru the asm calls X without setting r4 although X reads r4, and the C passes fewer arguments
           (constructor / base-constructor argument pass-through)
  scratch  scratch files (tmp_*, *.s/*.txt dumps) outside build/ and the scratchpad: in the repo and
           next to it
Suppress an intentional finding with a `lint:ok` comment on that line.
Run by tools/check.py after a successful compile. Warnings only; exit code 1 if any finding.
"""
import glob, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

MARKER_BAD = re.compile(r"^\s*//\s*0x80[0-9A-Fa-f]{6}[ \t]+(?![(-])\S")
MARKER = re.compile(r"^\s*//\s*0x(80[0-9A-Fa-f]{6})\s*$")
DECL = re.compile(r'^[ \t]*(?:extern "C"[ \t]+)?([A-Za-z_][\w \t\*]*?[ \t\*])(\w+)\(([^;{}]*)\)\s*;', re.M)
DEF = re.compile(r'^(?:extern "C"[ \t]+)?([A-Za-z_][\w \t\*:<>]*?[ \t\*])(\w+)\(([^;{}]*)\)\s*\{', re.M)
SKIP_WORDS = {"if", "while", "for", "switch", "return", "else", "do", "sizeof"}


def files_to_check(args):
    if "--all" in args:
        return sorted(p for p in glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)
                      if os.sep + "leaf" + os.sep not in p)
    explicit = [os.path.abspath(a) for a in args if not a.startswith("--")]
    if explicit:
        return explicit
    out = subprocess.run(["git", "status", "--porcelain"], cwd=ROOT, capture_output=True, text=True).stdout
    paths = [l[3:].strip().strip('"') for l in out.splitlines()]
    return [os.path.join(ROOT, p) for p in paths
            if p.endswith(".cpp") and p.startswith("src/") and "/leaf/" not in p
            and os.path.exists(os.path.join(ROOT, p))]


_index = None


def decl_index():
    """name -> (return type, param count) from every non-leaf prototype and definition."""
    global _index
    if _index is None:
        _index = {}
        paths = glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True) + \
            glob.glob(os.path.join(ROOT, "include", "**", "*.h"), recursive=True)
        paths.sort(key=lambda p: os.sep + "leaf" + os.sep in p)
        for p in paths:
            txt = re.sub(r"/\*.*?\*/", "", open(p, encoding="utf-8", errors="replace").read(), flags=re.S)
            txt = re.sub(r"//[^\n]*", "", txt)
            for rx in (DECL, DEF):
                for m in rx.finditer(txt):
                    ret, name, params = re.sub(r"^static" + chr(92) + "s+", "", m.group(1).strip()), m.group(2), m.group(3).strip()
                    if ret.split()[0] in SKIP_WORDS or "=" in ret:
                        continue
                    n = 0 if params in ("", "void") else params.count(",") + 1
                    _index.setdefault(name, (ret, n))
    return _index


# ---------------------------------------------------------------- asm helpers
_asm_fn = None
_asm_text = {}


def asm_function(addr):
    """Text of the function at `addr` (hex string) or None."""
    global _asm_fn
    import decomp
    if _asm_fn is None:
        _asm_fn = decomp.fn_index()
    hit = _asm_fn.get(addr.upper())
    if not hit:
        return None
    f, name = hit
    if f not in _asm_text:
        _asm_text[f] = open(f, encoding="utf-8", errors="replace").read()
    m = re.search(rf"^\.fn {re.escape(name)},.*?^\.endfn {re.escape(name)}", _asm_text[f], re.S | re.M)
    return m.group(0) if m else None


def instrs(text):
    out = []
    for l in text.splitlines():
        m = re.match(r"^\s*(?:/\*[^*]*\*/)?\s*(?:[0-9A-F]{8}\s+)?([a-z][\w.+-]*)\s*(.*?)\s*(?:#.*)?$", l)
        if m and not l.startswith((".fn", ".endfn", ".L_", "/*")) or (m and l.lstrip().startswith("/*")):
            out.append((m.group(1), m.group(2)))
    return out


def tail_target(text):
    """Name X if the function ends with `b X` (a tail call to another function)."""
    ins = instrs(text)
    for op, args in reversed(ins):
        if op == "b" and not args.startswith(".L_"):
            return args.split()[0]
        if op in ("blr", "bctr"):
            return None
        if op.startswith("b") and op not in ("bl",):
            continue
    return None


def first_call_without_r4(text):
    """(callee, True) if the first `bl` runs with r4 never written since entry (pass-through)."""
    written = False
    for op, args in instrs(text):
        if op == "bl":
            return args.split()[0], not written
        if op in ("stw", "stwu", "stfd", "stfs", "stmw", "mflr", "cmpwi", "cmplwi", "cmpw", "cmplw", "b", "beq", "bne", "blt",
                  "bgt", "ble", "bge", "mtlr", "mtctr", "crclr", "crset", "stb", "sth"):
            continue
        dest = args.split(",")[0].strip() if args else ""
        if dest == "r4" or dest == "r5":
            written = True
    return None, False


def reads_r4_early(text):
    for op, args in instrs(text)[:25]:
        parts = [x.strip() for x in args.split(",")]
        if op in ("stw", "stb", "sth", "stfs", "stfd"):
            if parts and parts[0] == "r4":
                return True
            continue
        if op.startswith("cmp"):
            if any(x == "r4" for x in parts):
                return True
            continue
        if parts and parts[0] == "r4" and op not in ("mr",):
            return False
        if op == "mr" and len(parts) == 2 and parts[1] == "r4":
            return True
        if op == "mr" and parts and parts[0] == "r4":
            return False
        if any(x == "r4" for x in parts[1:]):
            return True
        if op == "bl":
            return False
    return False


# ---------------------------------------------------------------- checks
def split_functions(txt):
    """Yield (marker_line, addr, ret, name, params, body_start_line, body) for `// 0xADDR` + definition."""
    lines = txt.split("\n")
    for i, l in enumerate(lines):
        m = MARKER.match(l)
        if not m:
            continue
        j = i + 1
        while j < len(lines) and (lines[j].strip().startswith("//") or not lines[j].strip()):
            j += 1
        head = "\n".join(lines[j:j + 4])
        d = DEF.match(head)
        if not d:
            continue
        depth, k, body = 0, j, []
        started = False
        while k < len(lines):
            for ch in lines[k]:
                if ch == "{":
                    depth += 1
                    started = True
                elif ch == "}":
                    depth -= 1
            body.append(lines[k])
            k += 1
            if started and depth == 0:
                break
        yield i + 1, m.group(1).upper(), d.group(1).strip(), d.group(2), d.group(3), j + 1, "\n".join(body)


def same_target(tgt, body, aliases=None):
    """True if the C body calls `tgt` or another name for the same address (aliases)."""
    if re.search(r"\b" + re.escape(tgt) + r"\b", body):
        return True
    import decomp
    by_addr, by_name = decomp.load_symbols()
    addr = by_name.get(tgt)
    if not addr:
        return False
    aliases = aliases or {}
    return any((by_name.get(n) or aliases.get(n)) == addr for n in set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", body)))


_SYMS = None
def symbol_names():
    """-> address -> name from config/RM7P4F/symbols.txt."""
    global _SYMS
    if _SYMS is None:
        _SYMS = {}
        path = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")
        if os.path.exists(path):
            for l in open(path, encoding="utf-8", errors="replace"):
                m = re.match(r"(\w+) = \.\w+:0x([0-9A-Fa-f]{8});", l)
                if m: _SYMS.setdefault(int(m.group(2), 16), m.group(1))
    return _SYMS

def marker_names_other(lines, n):
    """Line index n holds a marker: the declaration port_link would take for it instead of the
    definition, or None."""
    addr = int(MARKER.match(lines[n]).group(1), 16)
    for nxt in lines[n + 1:n + 6]:
        code = re.sub(r"//.*", "", nxt).strip()
        if not code:
            continue
        is_decl = code.endswith(";") or re.match(r"(typedef|struct|class|union|using)\b", code)
        if not is_decl:
            return None
        name = symbol_names().get(addr)
        m = re.search(r"(\w+)\s*\(", code)
        if m and (m.group(1) == name or m.group(1).upper().endswith("%08X" % addr)):
            return None          # a declaration of the function itself (lists of callees with markers)
        return code
    return None

def lint_file(path):
    rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
    txt = open(path, encoding="utf-8", errors="replace").read()
    out = []
    idx = decl_index()
    lines = txt.split("\n")
    aliases = {m.group(1): m.group(2).upper() for m in re.finditer(r'(\w+)\([^)]*\);\s*//\s*0x(8[0-9A-Fa-f]{7})', txt)}
    ok_lines = {n for n, l in enumerate(lines, 1) if "lint:ok" in l}
    for n, l in enumerate(lines, 1):
        if MARKER_BAD.match(l):
            out.append((n, "marker", "`// 0xADDR` must be alone on its line (put the description on the next line)"))
        if MARKER.match(l):
            other = marker_names_other(lines, n - 1)
            if other:
                out.append((n, "markname", "the first code after this marker is `%s`: tools/port_link.py would bind the address to it; move it above the marker" % other[:60]))
        s = re.sub(r"//.*", "", l)
        # src/port/ maps Wii addresses to host memory on purpose (docs/port_runtime.md).
        if not rel.startswith("src/port/") and re.search(r"\((?:u32|s32|BOOL|int|u16)\)\s*\(?\s*uintptr_t\)?", s) and "uintptr_t" in s:
            out.append((n, "ptrcast", "integer cast of a pointer via uintptr_t truncates on x64; widen the type (ScriptValue/intptr_t)"))
        elif re.search(r"\((?:u32|s32|BOOL|int)\)\s*&\w", s):
            out.append((n, "ptrcast", "address cast to a 32-bit integer truncates on x64"))
        elif re.search(r"\*\((?:u32|s32)\*\)[^=;]*=\s*(?:&\w|\(u8\*\)|\(void\*\)|\(char\*\))", s):
            out.append((n, "ptrcast", "pointer stored through a 32-bit slot truncates on x64 (use *(void**)...)"))
        else:
            m = re.search(r"\((?:u32|s32|BOOL|int)\)\s*\(?\s*(\w+)\(", s)
            if m and m.group(1) in idx and idx[m.group(1)][0].rstrip().endswith("*"):
                out.append((n, "ptrcast", f"{m.group(1)}() returns a pointer ({idx[m.group(1)][0]}); the 32-bit cast truncates it on x64"))
    for mline, addr, ret, name, params, bline, body in split_functions(txt):
        text = asm_function(addr)
        if not text:
            continue
        tgt = tail_target(text)
        body_nc = re.sub(r"//[^\n]*", "", body)
        stmts = [x.strip() for x in re.split(r"[;{}]", body_nc) if x.strip()]
        if tgt:
            callee_ret = idx.get(tgt, (None, 0))[0]
            last = stmts[-1] if stmts else ""
            returns_val = ret.strip() != "void"
            if ret.strip() == "void" and callee_ret and callee_ret.strip() != "void":
                out.append((bline, "tail", f"{name}: asm tail-calls {tgt} (returns {callee_ret}); the C function is void, so the value is dropped"))
            elif returns_val and re.match(rf"{re.escape(tgt)}\s*\(", last):
                out.append((bline, "tail", f"{name}: asm tail-calls {tgt}; write `return {tgt}(...)`"))
            elif not same_target(tgt, body_nc, aliases):
                out.append((bline, "tail", f"{name}: asm ends with a tail call to {tgt}, which the C never calls"))
        callee, passthru = first_call_without_r4(text)
        if callee and passthru and callee in idx and idx[callee][1] >= 2:
            pass
        if callee and passthru and callee in idx:
            ctext = None
            symaddr = None
            try:
                import decomp
                by_addr, by_name = decomp.load_symbols()
                symaddr = by_name.get(callee)
            except Exception:  # noqa: BLE001
                pass
            if symaddr:
                ctext = asm_function(symaddr)
            if ctext and reads_r4_early(ctext):
                m = re.search(rf"\b{re.escape(callee)}\(([^;]*)\)\s*;", body)
                nargs = 0
                if m and m.group(1).strip():
                    depth, nargs = 0, 1
                    for ch in m.group(1):
                        depth += ch == "("
                        depth -= ch == ")"
                        if ch == "," and depth == 0:
                            nargs += 1
                if m and nargs < 2:
                    out.append((bline, "passthru", f"{name}: asm calls {callee} without setting r4 and {callee} reads r4, "
                                                  f"but the C passes {nargs} argument(s): take and forward the extra argument(s)"))
    return rel, [f for f in out if f[0] not in ok_lines]


def scratch_files():
    out = []
    parent = os.path.dirname(ROOT)
    for pat in ("tmp_*", "*_asm.txt", "*_dump.txt"):
        for p in glob.glob(os.path.join(parent, pat)):
            if os.path.isfile(p):
                out.append(p)
    st = subprocess.run(["git", "status", "--porcelain", "--untracked-files=all"], cwd=ROOT, capture_output=True, text=True).stdout
    for l in st.splitlines():
        if l.startswith("??"):
            p = l[3:].strip().strip('"')
            if p.startswith("build/") or p.startswith("src/") or p.startswith("tools/") or p.startswith("docs/") \
                    or p.startswith("include/") or p.startswith("config/") or p.startswith("third_party/"):
                continue
            if re.search(r"(tmp|scratch|dump|asm)", os.path.basename(p), re.I) or p.endswith((".s", ".txt", ".asm")):
                out.append(os.path.join(ROOT, p))
    return out


def main():
    files = files_to_check(sys.argv[1:])
    total = 0
    for f in files:
        rel, findings = lint_file(f)
        for n, kind, msg in findings:
            print(f"{rel}:{n}: [{kind}] {msg}")
            total += 1
    for p in scratch_files():
        print(f"{p}: [scratch] scratch/dump file outside src/ and build/: delete it (never commit game data)")
        total += 1
    print(f"lint: {len(files)} files, {total} finding(s)")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
