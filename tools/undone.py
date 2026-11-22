"""List game functions that have no `// 0xADDR` marker yet (outside src/leaf), to pick decompile batches.

Usage: python tools/undone.py [--min 0x0] [--max 0xA00] [--count 60] [--batch 20]
                              [--renderer] [--missing-first] [--smallest] [--no-prs] [--exclude name,name]
Prints a summary, then the picked functions in batches (one line per batch, ready for
`python tools/draft_batch.py --names ... --name draft_missN`).
  largest first among those in the size range; `--missing-first` puts functions listed in
  docs/data/missing.json (new callees) first; renderer range 0x80150000-0x8018B000 is skipped
  unless `--renderer`; markers added by open PRs (gh pr diff) are excluded unless `--no-prs`.
Functions that only exist as generated src/leaf wrappers are skipped. A marker must be ALONE on its line, otherwise the function counts as undone."""
import re, glob, os, sys, json, argparse, subprocess, collections

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ap = argparse.ArgumentParser()
ap.add_argument('--min', type=lambda x: int(x, 0), default=0)
ap.add_argument('--max', type=lambda x: int(x, 0), default=0xA00)
ap.add_argument('--count', type=int, default=60)
ap.add_argument('--batch', type=int, default=20)
ap.add_argument('--renderer', action='store_true')
ap.add_argument('--missing-first', action='store_true')
ap.add_argument('--no-prs', action='store_true')
ap.add_argument('--smallest', action='store_true', help='pick the smallest functions first instead of the largest')
ap.add_argument('--exclude', default='')
a = ap.parse_args()

marks, leaf = set(), set()
for f in glob.glob(os.path.join(root, 'src', '**', '*.cpp'), recursive=True):
    is_leaf = os.sep + 'leaf' + os.sep in f
    txt = open(f, errors='ignore').read()
    for m in re.finditer(r'^\s*//\s*0x([0-9A-Fa-f]{8})\s*$', txt, re.M):
        (leaf if is_leaf else marks).add(int(m.group(1), 16))

if not a.no_prs:
    try:
        nums = subprocess.run(['gh', 'pr', 'list', '--state', 'open', '--json', 'number', '-q', '.[].number'],
                              capture_output=True, text=True, cwd=root).stdout.split()
        for n in nums:
            d = subprocess.run(['gh', 'pr', 'diff', n], capture_output=True, text=True, cwd=root).stdout
            for m in re.finditer(r'^\+\s*//\s*0x([0-9A-Fa-f]{8})\s*$', d, re.M):
                marks.add(int(m.group(1), 16))
    except Exception as e:
        print('warning: could not read open PRs:', e)

fns = []
for l in open(os.path.join(root, 'config', 'RM7P4F', 'symbols.txt')):
    m = re.match(r'(\S+) = \.text:0x([0-9A-F]{8}); // type:function size:0x([0-9A-F]+)', l)
    if m:
        ad = int(m.group(2), 16)
        if 0x80006B40 <= ad < 0x8022FEC4:
            fns.append((ad, m.group(1), int(m.group(3), 16)))
nl = [x for x in fns if x[0] not in marks and x[0] not in leaf]  # leaf = generated wrappers, not hand-decompiled
print('total', len(fns), 'marked', len([x for x in fns if x[0] in marks]), 'undone (non-leaf)', len(nl), 'bytes', sum(x[2] for x in nl))
print('by 4KB region:', sorted(collections.Counter(x[0] >> 12 for x in nl).items(), key=lambda t: -t[1])[:10])

skip = set(a.exclude.split(',')) if a.exclude else set()
pool = [x for x in nl if a.min <= x[2] <= a.max and x[1] not in skip
        and (a.renderer or not (0x80150000 <= x[0] < 0x8018B000))]
pool.sort(key=lambda x: x[2] if a.smallest else -x[2])
if a.missing_first:
    try:
        d = json.load(open(os.path.join(root, 'docs', 'data', 'missing.json')))
        miss = [(g['name'] if isinstance(g, dict) else g) for g in d['game']]
        pool.sort(key=lambda x: (x[1] not in miss, x[2] if a.smallest else -x[2]))
    except Exception as e:
        print('warning: no missing.json:', e)
pick = sorted(pool[:a.count], key=lambda x: x[0])
print('picked', len(pick), 'of pool', len(pool))
for i in range(0, len(pick), a.batch):
    print(' '.join(x[1] for x in pick[i:i + a.batch]))
