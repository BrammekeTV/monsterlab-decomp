"""Generate C++ for "hash table initializer" functions: sequences of
    slot = StringHash("name");
where both the slots and the name strings live in one static data block.

    python tools/gen_hashinit.py <asm function name> <C name> <0xaddr> [extra C lines...]

Prints the function to stdout. Strings are read from main.dol (tools/script_funcs.py).
"""
import re
import sys

sys.path.insert(0, "tools")
from script_funcs import cstr, _syms  # noqa: E402

HASH = ("StringHash", "fn_801EB704")


def body_of(name):
    import glob
    for f in glob.glob("build/RM7P4F/asm/*_text.s"):
        t = open(f).read()
        m = re.search(r"^\.fn " + re.escape(name) + r", \w+\n(.*?)^\.endfn", t, re.M | re.S)
        if m:
            return [re.sub(r"^/\*.*?\*/\s*", "", l).strip() for l in m.group(1).splitlines()]
    raise SystemExit("not found: " + name)


def main():
    asm_name, cname, addr = sys.argv[1:4]
    extra = sys.argv[4:]
    base = {}       # reg -> (symbol, offset)
    arg = None
    blocks = set()
    out = []
    for ins in body_of(asm_name):
        m = re.match(r"lis (r\d+), (\w+)@ha$", ins)
        if m:
            base[m.group(1)] = (m.group(2), 0)
            continue
        m = re.match(r"addi (r\d+), (r\d+), (\w+)@l$", ins)
        if m and m.group(2) in base:
            base[m.group(1)] = (m.group(3), 0)
            continue
        m = re.match(r"(addi|addis) (r\d+), (r\d+), (-?0x[0-9a-f]+)$", ins)
        if m and m.group(3) in base:
            s, o = base[m.group(3)]
            v = int(m.group(4), 16) << (16 if m.group(1) == "addis" else 0)
            if m.group(2) == "r3":
                arg = (s, o + v)
            else:
                base[m.group(2)] = (s, o + v)
            continue
        m = re.match(r"bl (\S+)$", ins)
        if m and m.group(1) in HASH:
            continue
        m = re.match(r"stw r3, (0x[0-9a-f]+)\((r\d+)\)$", ins)
        if m and m.group(2) in base and arg is not None:
            s, o = base[m.group(2)]
            name = cstr(arg[0], arg[1]).replace("\\", "\\\\").replace('"', '\\"')
            blocks.add(s)
            out.append(f'    *(u32*)({s} + 0x{o + int(m.group(1), 16):X}) = StringHash("{name}");')
            arg = None
    print(f"// {addr}")
    print(f'extern "C" void {cname}(void) {{')
    for e in extra:
        print("    " + e)
    print("\n".join(out))
    print("}")
    print(f"// {len(out)} hashes; blocks: {', '.join(sorted(blocks))}", file=sys.stderr)


if __name__ == "__main__":
    main()
