"""Signatures of the decompiled functions, read from the sources, for the differential tests.

Every definition under a `// 0xADDRESS` marker gives: address, name, file, and a signature string for
src/port/port_difftest.cpp: return kind ':' argument kinds
    v nothing   z bool   b u8   c s8   h u16   s s16   i 32-bit integer   p pointer   f float   d double
A definition the harness cannot call (a struct by value, a 64-bit integer, a member function,
variable arguments) gets sig None and the reason.
"""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MARK = re.compile(r"^// 0x([0-9A-Fa-f]{8})\s*$")
KINDS = {
    "void": "v", "bool": "z",
    "u8": "b", "unsigned char": "b", "uint8_t": "b",
    "s8": "c", "char": "c", "signed char": "c", "int8_t": "c",
    "u16": "h", "unsigned short": "h", "uint16_t": "h", "wchar_t": "h",
    "s16": "s", "short": "s", "int16_t": "s",
    "u32": "i", "s32": "i", "int": "i", "unsigned": "i", "unsigned int": "i", "long": "i", "unsigned long": "i",
    "BOOL": "i", "size_t": "i", "uintptr_t": "i", "intptr_t": "i", "int32_t": "i", "uint32_t": "i", "ScriptValue": "i",
    "f32": "f", "float": "f", "f64": "d", "double": "d",
}
DROP = re.compile(r'extern\s+"C"|(const|volatile|struct|class|enum|register|static|inline|__cdecl)')


def kind(text):
    """Kind letter of one parameter or return type, or None with the type's name."""
    t = text.split("=")[0]
    if "*" in t or "&" in t or "[" in t or "(" in t:
        return "p", None
    t = DROP.sub(" ", t).split()
    for n in (len(t), len(t) - 1):                 # with and without the parameter's name
        if n > 0 and " ".join(t[:n]) in KINDS:
            return KINDS[" ".join(t[:n])], None
    return None, " ".join(t[:-1] if len(t) > 1 else t)


def split_params(text):
    out, depth, cur = [], 0, ""
    for ch in text:
        if ch in "(<[":
            depth += 1
        elif ch in ")>]":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur); cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return out


def parse(decl):
    """(name, sig, reason) from the text between the marker and the opening brace."""
    decl = re.sub(r"//[^\n]*", "", decl)
    decl = " ".join(decl.split())
    m = re.match(r"^(.*?)([A-Za-z_][\w:]*)\s*\((.*)\)\s*(const)?\s*$", decl)
    if not m:
        return None, None, "definition not understood"
    ret, name, params = m.group(1), m.group(2), m.group(3)
    if "::" in name:
        return name, None, "member function"
    rk, bad = kind(ret + " x") if ret.strip() else ("i", None)
    if rk is None:
        return name, None, f"returns {bad}"
    args = ""
    params = params.strip()
    if params and params != "void":
        for p in split_params(params):
            if "..." in p:
                return name, None, "variable arguments"
            k, bad = kind(p)
            if k is None or k == "v":
                return name, None, f"parameter of type {bad}"
            args += k
    return name, rk + ":" + args, None


def scan(dirs=("src/game", "src/engine", "src/system", "src/leaf")):
    """Every marked definition: list of dicts addr, name, sig, reason, file, generated."""
    out, seen = [], set()
    for d in dirs:
        base = os.path.join(ROOT, d)
        for dp, _, files in os.walk(base):
            for fn in sorted(files):
                if not fn.endswith(".cpp"):
                    continue
                path = os.path.join(dp, fn)
                lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
                for i, line in enumerate(lines):
                    m = MARK.match(line)
                    if not m:
                        continue
                    addr = int(m.group(1), 16)
                    decl, j = "", i + 1
                    while j < len(lines) and lines[j].lstrip().startswith("//"):   # the comment that describes the function
                        j += 1
                    end = j + 12
                    while j < len(lines) and j < end:
                        cut = lines[j].split("{")[0]
                        decl += " " + cut
                        if "{" in lines[j] or lines[j].rstrip().endswith(";"):
                            break
                        j += 1
                    name, sig, reason = parse(decl)
                    if addr in seen:
                        continue
                    seen.add(addr)
                    out.append({"addr": addr, "name": name or f"fn_{addr:08X}", "sig": sig, "reason": reason,
                                "file": os.path.relpath(path, ROOT).replace("\\", "/"), "generated": d == "src/leaf"})
    out.sort(key=lambda e: e["addr"])
    return out


def audited():
    """Addresses in docs/audit_log.txt."""
    s = set()
    try:
        for line in open(os.path.join(ROOT, "docs", "audit_log.txt"), encoding="utf-8", errors="replace"):
            m = re.match(r"^0x([0-9A-Fa-f]{8})\b", line)
            if m:
                s.add(int(m.group(1), 16))
    except OSError:
        pass
    return s
