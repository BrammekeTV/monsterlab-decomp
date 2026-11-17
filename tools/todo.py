"""List functions in asm objects with instruction counts and decompiled status.

    python tools/todo.py <asm file substring> [...]
"""
import glob, os, re, sys

done = set()
for p in glob.glob("src/**/*.cpp", recursive=True):
    for m in re.finditer(r"^// 0x([0-9A-Fa-f]{8})\s*$", open(p, encoding="utf-8").read(), re.M):
        done.add(m.group(1).upper())

for pat in sys.argv[1:]:
    for f in sorted(glob.glob(f"build/RM7P4F/asm/*{pat}*")):
        t = open(f).read()
        for m in re.finditer(r"^\.fn (\S+), \w+\n(.*?)^\.endfn", t, re.M | re.S):
            body = m.group(2)
            a = re.search(r"^/\* ([0-9A-F]{8}) ", body, re.M)
            if not a: continue
            n = len(re.findall(r"^/\* [0-9A-F]{8} ", body, re.M))
            calls = sorted(set(re.findall(r"\bbl (\S+)", body)))
            print(f"{a.group(1)} {m.group(1):28} {n:5} {'DONE' if a.group(1) in done else '    '} {' '.join(calls)[:90]}")
