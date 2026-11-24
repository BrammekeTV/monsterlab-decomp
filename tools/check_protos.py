#!/usr/bin/env python3
"""Report functions declared/defined with conflicting signatures across src/ and include/.

Scans for `ret name(params)` declarations and definitions at file scope and flags a name that
appears with different return types, parameter counts, or C vs C++ linkage (extern "C").
Declarations are also compared with the generated definitions in src/leaf/: fewer parameters than the
definition reads, or a return value that travels differently (float, struct, 64-bit), is a conflict;
more parameters is harmless and only counted (-v lists them).
Usage: python tools/check_protos.py [-v] [names...]   (exit code 1 if conflicts are found)
"""
import os, re, sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KEYWORDS = {"return", "else", "if", "while", "for", "switch", "sizeof", "case", "new", "delete",
            "do", "goto", "typedef", "defined", "operator"}
DECL = re.compile(
    r'^[ 	]*(?P<pre>(?:(?:extern[ 	]+"C"|extern|static|inline|const|volatile|struct|unsigned|signed)\s+)*)'
    r'(?P<ret>[A-Za-z_][\w:<>]*(?:[ 	]+[A-Za-z_]\w*)*?[ 	]*\**)(?:[ 	]+|(?<=\*))'
    r'(?P<name>[A-Za-z_]\w*)\s*\((?P<args>[^;{}()]*(?:\([^)]*\)[^;{}()]*)*)\)\s*(?P<end>[;{])',
    re.M)


def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group(0)), s, flags=re.S)
    return re.sub(r'//[^\n]*', '', s)


def linkage_map(s):
    """For each offset, True if inside an extern "C" { } block."""
    spans, stack = [], []
    for m in re.finditer(r'extern\s+"C"\s*\{|\{|\}', s):
        t = m.group(0)
        if t.startswith('extern'):
            stack.append(('C', m.start()))
        elif t == '{':
            stack.append(('b', m.start()))
        elif stack:
            k, st = stack.pop()
            if k == 'C':
                spans.append((st, m.start()))
    return spans


def norm_ret(r):
    r = re.sub(r'\b(extern|static|inline)\b', '', r)
    r = re.sub(r'\s+', ' ', r).strip().replace(' *', '*')
    # Same ABI class on every target: all pointers; 32-bit integers (signedness is only semantic).
    if r.endswith('*'):
        return 'ptr'
    return {'BOOL': 'int32', 'int': 'int32', 's32': 'int32', 'u32': 'int32', 'ScriptValue': 'intptr'}.get(r, r)


def count_params(a):
    a = a.strip()
    if a in ('', 'void'):
        return 0
    depth, n = 0, 1
    for ch in a:
        depth += ch in '(<'
        depth -= ch in ')>'
        if ch == ',' and depth == 0:
            n += 1
    return n


def scan():
    sigs = defaultdict(list)
    for base in ('src', 'include'):
        for dp, _, fs in os.walk(os.path.join(ROOT, base)):
            for f in fs:
                if not f.endswith(('.cpp', '.h', '.c', '.hpp')):
                    continue
                path = os.path.join(dp, f)
                raw = open(path, encoding='utf-8', errors='replace').read()
                s = strip_comments(raw)
                spans = linkage_map(s)
                c_names = set()   # declared extern "C" earlier in this file: later redeclarations keep C linkage
                for m in DECL.finditer(s):
                    name = m.group('name')
                    ret = norm_ret(m.group('ret'))
                    if name in KEYWORDS or name == "ORIG" or ret in KEYWORDS or ret.startswith(("default", "case")) or ret.startswith('static') or \
                            re.search(r'\bstatic\b', s[m.start():m.start('ret')]):
                        continue
                    if m.group('end') == ';' and '=' in m.group('args'):
                        continue
                    # skip statements like `x = foo(a);` / calls: require a return type token
                    if not re.match(r'^[A-Za-z_]', ret):
                        continue
                    line = s.count('\n', 0, m.start('name')) + 1
                    explicit = '"C"' in m.group('pre') or 'extern "C"' in s[max(0, m.start() - 12):m.start()] or \
                        any(a <= m.start() < b for a, b in spans) or name in c_names
                    if explicit:
                        c_names.add(name)
                    sigs[name].append((ret, count_params(m.group('args')), explicit,
                                       os.path.relpath(path, ROOT).replace(os.sep, '/'), line,
                                       m.group('end') == '{'))
    return sigs


INT_RETS = {'ptr', 'int32', 'intptr', 'u8', 's8', 'u16', 's16', 'bool', 'char', 'long', 'short'}
FLOAT_RETS = {'f32', 'f64', 'float', 'double'}


def ret_class(r):
    """How a return value travels: only differences between these classes change the x86 call."""
    if r == 'void':
        return 'void'
    if r in INT_RETS:
        return 'int'
    if r in FLOAT_RETS:
        return 'float'
    if r in ('s64', 'u64'):
        return 'int64'
    return 'struct'


def leaf_check(v):
    """Compare hand-written declarations with the generated definition in src/leaf/.

    The generator counts the argument registers the code reads, so a declaration with fewer parameters
    leaves the callee reading stray stack words on x86 (PowerPC reads the registers whatever was declared).
    More parameters than the leaf reads is harmless: the callee ignores them.
    Returns (failures, notes)."""
    leaf = [x for x in v if x[3].startswith('src/leaf/') and x[5]]
    if not leaf:
        return [], []
    lret, ln = leaf[0][0], leaf[0][1]
    fails, notes = [], []
    for ret, n, ext, p, line, d in v:
        if p.startswith('src/leaf/'):
            continue
        if n < ln:
            fails.append(f'{p}:{line}  {n} params, the src/leaf definition reads {ln}')
        elif n > ln:
            notes.append(f'{p}:{line}  {n} params, the src/leaf definition reads {ln}')
        a, b = ret_class(ret), ret_class(lret)
        if a != b and ({a, b} & {'float', 'struct', 'int64'}):
            fails.append(f'{p}:{line}  returns {ret}, the src/leaf definition returns {lret}')
    return fails, notes


def main():
    only = set(a for a in sys.argv[1:] if not a.startswith('-'))
    verbose = '-v' in sys.argv[1:]
    sigs = scan()
    bad = 0
    more = []
    hdr = {n for n, v in sigs.items() if any(x[3].endswith('script_callbacks.h') for x in v)}
    for name in sorted(sigs):
        if only and name not in only:
            continue
        v = sigs[name]
        hand = [x for x in v if not x[3].startswith('src/leaf/')]
        rets = {x[0] for x in hand}
        cnts = {x[1] for x in hand}
        # script_callbacks.h is always included inside extern "C" by its users: its linkage is moot.
        links = {x[2] or name in hdr for x in v if not x[3].endswith('script_callbacks.h')}
        why = []
        if len(rets) > 1:
            why.append('return types ' + ', '.join(sorted(rets)))
        if len(cnts) > 1:
            why.append('param counts ' + ', '.join(map(str, sorted(cnts))))
        if len(links) > 1:
            why.append('C and C++ linkage')
        fails, notes = leaf_check(v)
        if fails:
            why.append('differs from the src/leaf definition')
        more += [f'{name}: {n}' for n in notes]
        if why:
            bad += 1
            print(f'{name}: {"; ".join(why)}')
            for ret, n, ext, p, ln, d in v:
                print(f'    {p}:{ln}  {ret} ({n} params){" extern C" if ext else ""}{" [def]" if d else ""}')
            for f in fails:
                print(f'    -> {f}')
    if more:
        print(f'{len(more)} declaration(s) with more parameters than the src/leaf definition reads (harmless'
              + ('):' if verbose else '; -v lists them)'))
        if verbose:
            for n in more:
                print('    ' + n)
    print(f'{bad} conflicting function(s)')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
