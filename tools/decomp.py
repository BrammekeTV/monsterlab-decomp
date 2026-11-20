"""Draft C for functions with m2c (https://github.com/matt-kempster/m2c).

    python tools/decomp.py <name or 0xADDR> [...] [--stdout]

Writes build/m2c/<name>.c. The draft is a starting point: fix types, field names and logic by
hand before it goes into src/. Float/string constants are inlined from the data sections, and
auto names (fn_/lbl_XXXXXXXX) are replaced with the current names from symbols.txt.

Setup (once): git clone https://github.com/matt-kempster/m2c tools/m2c
              python -m pip install "graphviz~=0.20.1"
"""
import glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASM = os.path.join(ROOT, "build", "RM7P4F", "asm")
M2C = os.path.join(ROOT, "tools", "m2c", "m2c.py")
SYMBOLS = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")
OUT = os.path.join(ROOT, "build", "m2c")
DATA = ["*_rodata.s", "*_sdata.s", "*_sdata2.s", "*_data.s"]
CONTEXT = os.path.join(ROOT, "tools", "m2c_context.h")
CALLBACKS = os.path.join(ROOT, "include", "game", "script_callbacks.h")
BASIC = {"void", "s8", "s16", "s32", "s64", "u8", "u16", "u32", "u64", "f32", "f64", "BOOL", "char", "int"}


def build_context():
    """tools/m2c_context.h plus the script callback prototypes, made C-compatible."""
    base = open(CONTEXT, encoding="utf-8").read()
    known = set(re.findall(r"\b(\w+)\s*\(", base)) | set(re.findall(r"typedef struct \w+ \{.*?\} (\w+);", base, re.S))
    types = BASIC | set(re.findall(r"\} (\w+);", base))

    def ctype(s):
        s = re.sub(r"/\*.*?\*/", "", s).strip()
        s = s.replace("const ", "")
        m = re.match(r"(\w+)\s*(\**)", s)
        if not m:
            return None
        base_t, stars = m.group(1), m.group(2)
        if base_t not in types:
            return "void *"
        return base_t + (" " + stars if stars else "")

    extra = []
    # Prototypes we wrote in extern "C" blocks of the decompiled sources (first one wins).
    lines = []
    for p in sorted(glob.glob(os.path.join(ROOT, "include", "**", "*.h"), recursive=True)) + \
             sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        if os.sep + "leaf" + os.sep in p:
            continue
        inside = False
        for line in open(p, encoding="utf-8", errors="replace"):
            s = line.strip()
            if s.startswith('extern "C" {'):
                inside = True
                continue
            if inside and s.startswith("}"):
                inside = False
                continue
            if s.startswith('extern "C" ') and s.endswith(";"):
                lines.append(s[len('extern "C" '):])
            elif inside:
                lines += [x.strip() + ";" for x in re.sub(r"//.*", "", s).split(";") if "(" in x]
    lines += list(open(CALLBACKS, encoding="utf-8"))
    for line in lines:
        if "(*" in line or "operator" in line:
            continue
        m = re.match(r"^(.+?)\b(\w+)\((.*)\);", line)
        if not m or line.startswith("extern") or m.group(2) in known or m.group(2) in ("sizeof", "memcpy", "alignof", "if", "while", "for", "switch", "return"):
            continue
        ret = ctype(m.group(1))
        params = re.sub(r"/\*.*?\*/", "", m.group(3)).strip()
        ps = [] if params in ("", "void") else [ctype(re.sub(r"\s*\b\w+$", "", p.strip())) for p in params.split(",")]
        if ret is None or None in ps:
            continue
        extra.append(f"{ret} {m.group(2)}({', '.join(ps) or 'void'});")
        known.add(m.group(2))
    path = os.path.join(OUT, "_context.h")
    os.makedirs(OUT, exist_ok=True)
    open(path, "w", encoding="utf-8").write(base + "\n/* script callbacks */\n" + "\n".join(extra) + "\n")
    return path


def load_symbols():
    by_addr, by_name = {}, {}
    for line in open(SYMBOLS, encoding="utf-8"):
        m = re.match(r"(\S+) = \.\w+:0x([0-9A-Fa-f]{8});", line)
        if m:
            by_addr[m.group(2).upper()] = m.group(1)
            by_name[m.group(1)] = m.group(2).upper()
    return by_addr, by_name


_FN_INDEX = None


def fn_index():
    """Map function start address -> (asm file, function name), built once per run."""
    global _FN_INDEX
    if _FN_INDEX is None:
        _FN_INDEX = {}
        for f in glob.glob(os.path.join(ASM, "*_text.s")) + glob.glob(os.path.join(ASM, "*init*.s")):
            text = open(f, encoding="utf-8", errors="replace").read()
            for m in re.finditer(r"^\.fn (\S+), \w+\n(?:[^\n]*\n)*?/\* ([0-9A-F]{8}) ", text, re.M):
                _FN_INDEX.setdefault(m.group(2), (f, m.group(1)))
    return _FN_INDEX


def find_asm(addr, names):
    hit = fn_index().get(addr)
    if hit:
        return hit
    for f in glob.glob(os.path.join(ASM, "*_text.s")) + glob.glob(os.path.join(ASM, "*init*.s")):
        text = open(f, encoding="utf-8", errors="replace").read()
        for n in names:
            if re.search(rf"^\.fn {re.escape(n)},", text, re.M):
                return f, n
        if re.search(rf"^/\* {addr} ", text, re.M):
            m = list(re.finditer(r"^\.fn (\S+),", text[: text.index(f"/* {addr} ")], re.M))
            if m:
                return f, m[-1].group(1)
    return None, None


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    to_stdout = "--stdout" in sys.argv
    if not args:
        print(__doc__)
        return 1
    by_addr, by_name = load_symbols()
    data = sorted({p for g in DATA for p in glob.glob(os.path.join(ASM, g))})
    os.makedirs(OUT, exist_ok=True)
    ctx = build_context()

    def rename(m):
        return by_addr.get(m.group(2).upper(), m.group(0))

    for a in args:
        if a.lower().startswith("0x"):
            addr = a[2:].upper()
        else:
            addr = by_name.get(a)
            if addr is None:
                print(f"{a}: not in symbols.txt", file=sys.stderr)
                continue
        names = {f"fn_{addr}", by_addr.get(addr, "")} - {""}
        asm, fn = find_asm(addr, names)
        if asm is None:
            print(f"{a}: function not found in asm", file=sys.stderr)
            continue
        r = subprocess.run([sys.executable, M2C, "-t", "ppc-mwcc-c", "--context", ctx, "-f", fn, asm, *data],
                           capture_output=True, text=True)
        src = r.stdout if r.returncode == 0 else r.stdout + r.stderr
        src = re.sub(r"\b(fn|lbl)_([0-9A-Fa-f]{8})\b", rename, src)
        src = f"// m2c draft of {by_addr.get(addr, fn)} (0x{addr}) from {os.path.basename(asm)}\n" + src
        if to_stdout:
            print(src)
        else:
            path = os.path.join(OUT, f"{by_addr.get(addr, fn)}.c")
            open(path, "w", encoding="utf-8").write(src)
            print(f"{path}: {src.count(chr(10))} lines" + ("" if r.returncode == 0 else " (m2c error)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
