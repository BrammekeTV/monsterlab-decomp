"""Function histogram of a MONSTERLAB_SAMPLE profile (see port_overlay.cpp).

  MONSTERLAB_SAMPLE=build/pc/samples.txt build/pc/monsterlab.exe --to-menu --fast --frames 3800 --screenshot x.png
  python tools/resolve_samples.py [samples.txt] [monsterlab.map] [top]
"""
import re, sys, bisect, collections

samples = sys.argv[1] if len(sys.argv) > 1 else "build/pc/samples.txt"
mapf = sys.argv[2] if len(sys.argv) > 2 else "build/pc/monsterlab.map"
top = int(sys.argv[3]) if len(sys.argv) > 3 else 30
syms = []
for line in open(mapf, errors="replace"):
    m = re.match(r"\s*0001:([0-9a-f]+)\s+(\S+)\s+([0-9a-f]+)\s+(?:[fi]\s+)*(\S+)", line)
    if m:
        syms.append((int(m.group(3), 16) - 0x400000, m.group(2), m.group(4)))
syms.sort()
keys = [s[0] for s in syms]
hist, objs, total = collections.Counter(), collections.Counter(), 0
for line in open(samples):
    line = line.strip()
    if not line:
        continue
    if line.startswith("ext ntdll") or line.startswith("ext win32u") or line.startswith("ext KERNELBASE"):
        continue                                      # threads waiting their turn
    if line.startswith("ext "):
        hist["[" + line[4:] + "]"] += 1
        objs["(outside the exe)"] += 1
        total += 1
        continue
    a = int(line, 16)
    i = bisect.bisect_right(keys, a) - 1
    name, obj = (syms[i][1], syms[i][2]) if i >= 0 else ("?", "?")
    hist[name] += 1
    objs[obj] += 1
    total += 1
print(f"{total} samples")
for n, c in hist.most_common(top):
    print(f"{100.0 * c / total:5.1f}%  {n}")
print("-- by object")
for n, c in objs.most_common(10):
    print(f"{100.0 * c / total:5.1f}%  {n}")
