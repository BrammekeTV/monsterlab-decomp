"""Top functions of build/pc/samples.txt (PORT_SAMPLE=1 run) using build/pc/monsterlab.map. Usage: sample_report.py [N]"""
import re, sys, bisect, collections
syms = []
pat = re.compile(r'^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})')
base = None
for ln in open('build/pc/monsterlab.map', errors='ignore'):
    m = re.search(r'Preferred load address is ([0-9A-Fa-f]+)', ln)
    if m: base = int(m.group(1), 16)
    m = pat.match(ln)
    if m: syms.append((int(m.group(2), 16), m.group(1)))
syms.sort()
keys = [a for a, _ in syms]
tot = collections.Counter(); callers = collections.Counter(); other = 0; mods = []
for ln in open('build/pc/samples.txt'):
    if ln.startswith('#'): mods.append(ln[2:].strip()); continue
    a, c = ln.split()
    isc = a.startswith('C')
    if isc: a = a[1:]
    if a == 'other': other = int(c); continue
    va = int(a, 16) + base + 0x1000 * 0  # rva
    i = bisect.bisect_right(keys, va) - 1
    (callers if isc else tot)[syms[i][1] if i >= 0 else '?'] += int(c)
s = sum(tot.values()) + other
cs = sum(callers.values())
print('samples', s, 'in other/system', other)
for n, c in tot.most_common(int(sys.argv[1]) if len(sys.argv) > 1 else 25):
    print(f'{100.0*c/s:5.1f}%  {c:6d}  {n}')

print('system modules (name count):'); [print('  ' + m) for m in sorted(mods, key=lambda x: -int(x.rsplit(' ', 1)[1]))[:8]]

print('callers of system code (waits, heap, driver):')
for n, c in callers.most_common(25): print(f'{100.0*c/cs:5.1f}%  {c:6d}  {n}')
