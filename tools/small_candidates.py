"""List undecompiled game functions in a size range, grouped by 64 KB region.
Usage: python tools/small_candidates.py [min_size] [max_size] [regions] [per_region]"""
import re, glob, sys
lo = int(sys.argv[1], 0) if len(sys.argv) > 1 else 0x30
hi = int(sys.argv[2], 0) if len(sys.argv) > 2 else 0x60
nreg = int(sys.argv[3]) if len(sys.argv) > 3 else 3
per = int(sys.argv[4]) if len(sys.argv) > 4 else 20
done = set()
for f in glob.glob('src/**/*.c*', recursive=True):
    for m in re.finditer(r'^\s*//\s*0x(80[0-9A-Fa-f]{6})\s*$', open(f, encoding='utf-8', errors='replace').read(), re.M):
        done.add(int(m.group(1), 16))
regs = {}
for l in open('config/RM7P4F/symbols.txt'):
    m = re.match(r'(\S+) = \.text:0x([0-9A-F]{8}); // type:function size:0x([0-9A-F]+)', l)
    if not m: continue
    a, s = int(m.group(2), 16), int(m.group(3), 16)
    if 0x80006B40 <= a < 0x8022FEC4 and a not in done and lo <= s <= hi:
        regs.setdefault(a >> 16, []).append(m.group(1))
for r in sorted(regs, key=lambda k: -len(regs[k]))[:nreg]:
    print(f"{r:04X}", len(regs[r]), ' '.join(regs[r][:per]))
