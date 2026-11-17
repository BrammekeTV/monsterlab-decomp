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


def load_symbols():
    by_addr, by_name = {}, {}
    for line in open(SYMBOLS, encoding="utf-8"):
        m = re.match(r"(\S+) = \.\w+:0x([0-9A-Fa-f]{8});", line)
        if m:
            by_addr[m.group(2).upper()] = m.group(1)
            by_name[m.group(1)] = m.group(2).upper()
    return by_addr, by_name


def find_asm(addr, names):
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
        r = subprocess.run([sys.executable, M2C, "-t", "ppc-mwcc-c", "-f", fn, asm, *data],
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
