"""Missing-symbols report from the linker output of the link check.

    python tools/link_check.py                       # local: compile + link, writes build/link/link.log
    python tools/missing_symbols.py [link.log]       # parse it, write docs/data/missing.json + summary

Reads LNK2019/LNK2001 (unresolved), LNK2005 (duplicate definitions) and the linker's "could
potentially match" hints (a symbol that IS defined under another type or C/C++ linkage: those
are mismatches to fix in the sources, not missing code).  Each unresolved symbol is mapped to
its address/size in config/RM7P4F/symbols.txt and grouped (game code, zlib/SDK/MSL/Bink library
code by tools/progress.py ranges, globals), ranked by the number of referencing places.
Always exits 0: the link is allowed to fail.
"""
import collections, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import progress as P

ROOT = P.ROOT
UNRES = re.compile(r"^(?:.*[\/])?(\S+?)\.obj : error LNK(2019|2001): unresolved external symbol (.+?)(?: referenced in function (.+))?$")
DUP = re.compile(r"^(?:.*[\/])?(\S+?)\.obj : error LNK2005: (.+?) already defined in (?:.*[\/])?(\S+?)\.obj")
HINT = re.compile(r'^\s+"(.+)" \((\?.+)\)\s*$')
DECOR = re.compile(r"\((\?[^)]*)\)\s*$")
SYMLINE = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+); // type:(\w+) size:0x([0-9A-Fa-f]+)")


def base_name(sym):
    """('_foo' | '"decl" (?foo@@...)') -> ('foo', is_cpp_decorated)"""
    m = DECOR.search(sym)
    if m:
        return re.match(r"\?+(?:\$)?([^@]+)", m.group(1)).group(1), True
    return sym.lstrip("_") if sym.startswith("_") else sym, False


def lib_region(addr):
    for lo, hi, name in P.LIB_REGIONS:
        if lo <= addr < hi:
            return name
    return "other library"


def load_symbols():
    syms = {}
    for line in open(P.SYMS):
        m = SYMLINE.match(line)
        if m:
            syms.setdefault(m.group(1), (m.group(2), int(m.group(3), 16), m.group(4), int(m.group(5), 16)))
    return syms


def main():
    log = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else os.path.join(ROOT, "build", "link", "link.log")
    lines = open(log, encoding="utf-8", errors="replace").read().splitlines()
    syms = load_symbols()
    refs = collections.defaultdict(set)      # base name -> {(obj, function)}
    decl = {}                                # base name -> first raw symbol text
    hints = {}                               # base name -> defined-as text
    dups = collections.defaultdict(set)
    cur = None
    for l in lines:
        l = re.sub(r"\s+\[[^\]]*\.vcxproj\]\s*$", "", l)   # MSBuild appends the project
        m = UNRES.match(l)
        if m:
            obj, _, sym, fn = m.groups()
            name, cpp = base_name(sym)
            refs[name].add((obj, fn or "?"))
            decl.setdefault(name, sym.strip())
            cur = name
            continue
        m = DUP.match(l)
        if m:
            dups[base_name(m.group(2))[0]].update([m.group(1), m.group(3)])
            continue
        m = HINT.match(l)
        if m and cur:
            hints.setdefault(cur, m.group(1))

    entries = []
    for name, rs in refs.items():
        s = syms.get(name)
        e = {"name": name, "refs": len(rs), "files": len({o for o, _ in rs}), "used_as": decl[name]}
        if name in hints:
            e["kind"] = "mismatch"
            e["defined_as"] = hints[name]
        elif s is None:
            e["kind"] = "unknown"
        else:
            sec, addr, typ, size = s
            e.update(addr=f"{addr:08X}", size=size, section=sec)
            if typ == "function":
                e["kind"] = "game" if P.GAME_START <= addr < P.GAME_END else "library"
                if e["kind"] == "library":
                    e["region"] = lib_region(addr)
            else:
                e["kind"] = "global"
        entries.append(e)
    entries.sort(key=lambda e: (-e["refs"], e["name"]))

    by = collections.defaultdict(list)
    for e in entries:
        by[e["kind"]].append(e)
    out = {
        "log": os.path.basename(log),
        "counts": {k: len(v) for k, v in by.items()},
        "game_bytes": sum(e.get("size", 0) for e in by["game"]),
        "duplicates": [{"name": n, "files": sorted(f)} for n, f in sorted(dups.items())],
        "game": by["game"][:300], "library": by["library"][:300], "global": by["global"][:300],
        "mismatch": by["mismatch"], "unknown": by["unknown"][:200],
    }
    lib_regions = collections.Counter(e["region"] for e in by["library"])
    out["library_regions"] = dict(lib_regions.most_common())
    os.makedirs(P.OUT_DIR, exist_ok=True)
    with open(os.path.join(P.OUT_DIR, "missing.json"), "w", newline="\n") as fh:
        json.dump(out, fh, indent=1)

    print(f"unresolved symbols: {len(entries)}  " + "  ".join(f"{k}={len(v)}" for k, v in sorted(by.items())))
    print(f"duplicate definitions: {len(dups)}")
    print(f"missing game functions: {len(by['game'])} ({out['game_bytes']} bytes); library functions by region: {out['library_regions']}")
    for title, key, n in (("Most-referenced missing game functions", "game", 15), ("Most-referenced mismatches (fix in sources)", "mismatch", 10),
                          ("Most-referenced missing globals", "global", 10), ("Unknown symbols (not in symbols.txt)", "unknown", 10)):
        print(f"\n{title}:")
        for e in by[key][:n]:
            print(f"  {e['refs']:4d} refs  {e['name']:36s} {e.get('addr', '')} {e.get('size', '')} {('-> ' + e['defined_as']) if 'defined_as' in e else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
