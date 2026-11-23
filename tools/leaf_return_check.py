"""Lists generated leaf functions (src/leaf) whose float / non-float return type disagrees with a
declaration elsewhere in src. On x86 a float result comes back on the x87 stack: a caller that
declares another return type leaves it there (eight leaks turn every float result into NaN) and,
for a pointer or integer return, reads a register the function never set.

Usage: python tools/leaf_return_check.py   (from the repository root)
"""
import re, glob, collections, os

leaf = {}
for f in glob.glob('src/leaf/*.cpp'):
    for m in re.finditer(r'^extern "C" (\w[\w\s\*]*?)\s*\b(\w+)\(([^)]*)\) \{', open(f, encoding='utf-8').read(), re.M):
        leaf[m.group(2)] = (m.group(1).strip(), m.group(3), f)


def isf(t):
    return t in ('f32', 'f64', 'float', 'double')


decl = collections.defaultdict(set)
pat = re.compile(r'^\s*(?:extern\s+(?:"C"\s+)?)?([A-Za-z_][\w]*(?:\s*\*+)?)\s+\**\s*(\w+)\(([^;{)]*)\)\s*;', re.M)
files = glob.glob('src/**/*.cpp', recursive=True) + glob.glob('src/**/*.h', recursive=True) + glob.glob('include/**/*.h', recursive=True)
for f in files:
    parts = f.replace(os.sep, '/').split('/')
    if 'leaf' in parts:
        continue
    t = open(f, encoding='utf-8', errors='replace').read()
    for m in pat.finditer(t):
        n = m.group(2)
        if n in leaf:
            rt = m.group(1).replace(' ', '')
            if rt in ('return', 'else'):
                continue
            decl[n].add((rt, f.replace(os.sep, '/'), t.count('\n', 0, m.start()) + 1))
bad = []
for n, ds in decl.items():
    lr = leaf[n][0]
    for rt, f, l in ds:
        if isf(lr) != isf(rt):
            bad.append((n, lr, rt, f, l))
names = sorted(set(b[0] for b in bad))
print(len(names), 'leaf functions with a float/non-float return mismatch;', len(bad), 'declarations')
for n in names:
    print(n, leaf[n][0], '|', ', '.join(sorted(set(b[2] for b in bad if b[0] == n))), '|', len([b for b in bad if b[0] == n]), 'decl(s)')
