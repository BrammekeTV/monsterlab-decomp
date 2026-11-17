"""Rename symbols in config/RM7P4F/symbols.txt.

Usage:
    python tools/rename.py OLD NEW [OLD NEW ...]
    python tools/rename.py -f names.txt        # lines of "OLD NEW" (# comments ok)

OLD may be a symbol name (fn_8023696C) or a bare hex address (8023696C).
Re-run `tools/bin/dtk.exe dol split config/RM7P4F/config.yml build/RM7P4F`
afterwards to regenerate asm with the new names.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMS = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")
LINE_RE = re.compile(r"^(\S+)( = \S+:0x)([0-9A-Fa-f]+)(;.*)$")


def main(argv):
    pairs = []
    if argv[:1] == ["-f"]:
        for line in open(argv[1]):
            line = line.split("#")[0].split()
            if len(line) == 2:
                pairs.append(tuple(line))
    else:
        pairs = list(zip(argv[0::2], argv[1::2]))
    if not pairs:
        print(__doc__)
        return 1

    lines = open(SYMS).read().splitlines()
    names = {}
    for i, l in enumerate(lines):
        m = LINE_RE.match(l)
        if m:
            names[m.group(1)] = i
            names.setdefault(m.group(3).upper().lstrip("0X"), i)

    done = 0
    for old, new in pairs:
        key = old if old in names else old.upper()
        if key not in names:
            print(f"skip: {old} not found")
            continue
        if new in names and names[new] != names[key]:
            print(f"skip: {new} already exists")
            continue
        i = names[key]
        m = LINE_RE.match(lines[i])
        lines[i] = new + m.group(2) + m.group(3) + m.group(4)
        names[new] = i
        done += 1
    open(SYMS, "w", newline="\n").write("\n".join(lines) + "\n")
    print(f"renamed {done}/{len(pairs)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
