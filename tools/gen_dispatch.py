"""Generate the script call dispatcher (0x8010CD1C) from its assembly and the signature table.

    python tools/gen_dispatch.py      -> src/game/script_dispatch.cpp

For every case of the jump table (signature 0x91 + i) it reads, before the indirect call:
  lwz rN, off(r5)  -> integer/pointer argument N from args[off / 4]
  lfs fN, off(r5)  -> float argument from args[off / 4]
  stw rX, S(r1) + addi rN, r1, S  -> argument passed by pointer to a copy of args[...]
and after it: stw r3 / stfs f1 to the result slot (0xa8) or nothing (void).
Any pattern it doesn't understand stops the script, so the output is exact or nothing.
"""
import re
import sys

FUNC = ("ScriptDispatch", "fn_8010CD1C")
FIRST_SIG = 0x91


def load_asm():
    import glob
    for f in glob.glob("build/RM7P4F/asm/*_text.s"):
        t = open(f).read()
        for name in FUNC:
            m = re.search(r"^\.fn " + name + r", \w+\n(.*?)^\.endfn", t, re.M | re.S)
            if m:
                lines = []
                for line in m.group(1).splitlines():
                    a = re.match(r"^/\* ([0-9A-F]{8}) ", line)
                    ins = re.sub(r"^/\*.*?\*/\s*", "", line).strip()
                    if a:
                        lines.append((int(a.group(1), 16), ins))
                return int(name.split("_")[1], 16) if name.startswith("fn_") else None, lines
    raise SystemExit("dispatcher not found")


def jump_targets(base):
    t = open("build/RM7P4F/asm/auto_07_80353460_data.s").read()
    m = re.search(r"\.obj jumptable_8036CB60, global\n(.*?)\.endobj", t, re.S)
    return [base + int(x, 16) for x in re.findall(r"\+0x([0-9A-F]+)", m.group(1))]


def parse_case(lines, start):
    """Return (args: list of (kind, reg, value)), ret kind) for the case starting at `start`."""
    idx = next(i for i, (a, _) in enumerate(lines) if a == start)
    regval, slot, args = {}, {}, {}
    ret = "void"
    called = False
    for a, ins in lines[idx:]:
        if not called:
            m = re.match(r"lwz (r\d+), (0x[0-9a-f]+)\((?:r5|r9)\)$", ins)
            if m:
                regval[m.group(1)] = ("arg", int(m.group(2), 16) // 4)
                continue
            m = re.match(r"lfs (f\d+), (0x[0-9a-f]+)\((?:r5|r9)\)$", ins)
            if m:
                args[m.group(1)] = ("f32", int(m.group(2), 16) // 4)
                continue
            m = re.match(r"stw (r\d+), (0x[0-9a-f]+)\(r1\)$", ins)
            if m and m.group(1) in regval:
                slot[int(m.group(2), 16)] = regval.pop(m.group(1))[1]
                continue
            m = re.match(r"addi (r\d+), r1, (0x[0-9a-f]+)$", ins)
            if m:
                regval[m.group(1)] = ("ptr", int(m.group(2), 16))
                continue
            if ins == "bctrl":
                called = True
                for r, v in regval.items():
                    if re.fullmatch(r"r([3-9]|10)", r):
                        args[r] = v
                continue
            # any other instruction that writes a register invalidates what we knew about it
            m = re.match(r"\w+\.? (r\d+|f\d+),", ins)
            if m and not ins.startswith(("stw", "stfs", "cmp", "mtctr")):
                regval.pop(m.group(1), None)
                args.pop(m.group(1), None)
            continue
        if ins.startswith("stw r3, 0xa8(r1)"):
            ret = "u32"
        elif ins.startswith("stfs f1, 0xa8(r1)"):
            ret = "f32"
        if ins.startswith("b ") or ins.startswith("blr"):
            break
    return args, slot, ret


def main():
    sigs = {}
    for line in open("build/script_sigs.txt"):
        p = line.split()
        sigs[int(p[0], 16)] = (int(p[2], 16), [int(x, 16) for x in p[6:]])
    base, lines = load_asm()
    if base is None:
        base = 0x8010CD1C
    targets = jump_targets(base)
    # The asm also uses r4/r5/r6 as scratch for the table address; only keep registers that
    # hold argument values or stack-copy pointers.
    out_cases = []
    shared = {}
    for i, tgt in enumerate(targets):
        sig = FIRST_SIG + i
        if tgt in shared:
            out_cases.append((sig, shared[tgt]))
            continue
        first = next(ins for a, ins in lines if a == tgt)
        if first == "li r31, 0x0":      # jumps to the default branch: not handled here
            continue
        args, slot, ret = parse_case(lines, tgt)
        gprs = sorted((r for r, v in args.items() if r.startswith("r") and v[0] in ("arg", "ptr")),
                      key=lambda r: int(r[1:]))
        fprs = sorted((r for r in args if r.startswith("f")), key=lambda r: int(r[1:]))
        params, vals = [], []
        # order: walk the signature; int-like args take the next GPR, floats the next FPR
        argc = len(sigs.get(sig, (0, []))[1])
        gi = fi = 0
        ok = True
        for k in range(argc):
            t = sigs[sig][1][k]
            if t == 0x05:
                if fi >= len(fprs):
                    ok = False
                    break
                params.append("f32")
                vals.append(f"AsF32(args[{args[fprs[fi]][1]}])")
                fi += 1
            else:
                if gi >= len(gprs):
                    ok = False
                    break
                kind, v = args[gprs[gi]]
                if kind == "arg":
                    params.append("u32")
                    vals.append(f"args[{v}]")
                else:
                    params.append("u32*")
                    vals.append(f"&copy[{len([x for x in vals if x.startswith('&copy')])}]")
                gi += 1
        if not ok or gi != len(gprs) or fi != len(fprs):
            print(f"sig 0x{sig:X}: could not match registers {gprs + fprs} to signature {sigs.get(sig)}", file=sys.stderr)
            sys.exit(1)
        copies = [slot[s] for s in sorted(slot)]
        body = []
        if copies:
            body.append("u32 copy[] = { " + ", ".join(f"args[{c}]" for c in copies) + " };")
        call = f"(({ret} (*)({', '.join(params) or 'void'}))fn)({', '.join(vals)})"
        if ret == "u32":
            body.append(f"out = {call};")
        elif ret == "f32":
            body.append(f"out = AsU32({call});")
        else:
            body.append(f"{call};")
        shared[tgt] = body
        out_cases.append((sig, body))

    src = ["// Script call dispatcher: calls a registered script function with the arguments the",
           "// script engine collected, according to the function's signature (see",
           "// script_signatures.cpp). One case per game signature 0x91 - 0x187.",
           "// Generated by tools/gen_dispatch.py from the original jump table.",
           "// Original: ScriptDispatch 0x8010CD1C (auto_03_8010CD18_text.s)",
           '#include <string.h>',
           "",
           '#include "types.h"',
           "",
           "struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes",
           "extern ScriptFunc gScriptFuncs[];                       // 0x80484230",
           "",
           "static inline f32 AsF32(u32 v) { f32 f; memcpy(&f, &v, 4); return f; }",
           "static inline u32 AsU32(f32 f) { u32 v; memcpy(&v, &f, 4); return v; }",
           "",
           "// 0x8010CD1C",
           "// args[0] is the 'this' slot; script arguments start at args[1]. Returns FALSE for",
           "// signatures this dispatcher doesn't handle (the engine's own dispatcher does those).",
           'extern "C" BOOL ScriptDispatch(s32 func, u32 sig, u32* args, u32* result) {',
           "    u32 out = *result;",
           "    void* fn = gScriptFuncs[func].fn;",
           "    switch (sig) {"]
    for sig, body in out_cases:
        src.append(f"    case 0x{sig:X}: {{")
        src += ["        " + b for b in body]
        src.append("        break;")
        src.append("    }")
    src += ["    default:",
            "        return FALSE;",
            "    }",
            "    *result = out;",
            "    return TRUE;",
            "}",
            ""]
    open("src/game/script_dispatch.cpp", "w", encoding="utf-8").write("\n".join(src))
    print(len(out_cases), "cases")


if __name__ == "__main__":
    main()
