"""Differential tests: the decompiled C++ against the original PowerPC code (docs/difftest.md).

    python tools/difftest.py --fn <name|0xADDR> [--case K]     one function; --case K shows that case in full
    python tools/difftest.py --batch N                         N functions spread over the game
    python tools/difftest.py --all                             every marked function
    options: --calls        also functions that call decompiled game functions (default: leaf functions only)
             --cases N      inputs per function (default 40)       --seed N   (default 1)
             --audited / --unaudited   only functions in / not in docs/audit_log.txt
             --show-known / --strict   list / ignore tools/difftest/known_limits.txt (harness limits)
             --globals      some cases point zero global pointers (gBattle, ...) at the test's buffers (more functions run; the
                            C++ must keep that global at its original address, else a false MISMATCH)
             --files S      only functions whose source file path contains S (comma-separated)
             --no-leaf      leave out the generated src/leaf functions
             --iso PATH     the game disc (default: orig/RM7P4F)   --no-build

The original instructions are read from your own main.dol by build/pc/monsterlab.exe at test time
(src/port/port_difftest.cpp holds the PowerPC interpreter); nothing of the game is written anywhere.
The report goes to build/difftest/report.txt and report.json (not committed).
"""
import json, os, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "difftest"))
import sigs  # noqa: E402

EXE = os.path.join(ROOT, "build", "pc", "monsterlab.exe")
OUT = os.path.join(ROOT, "build", "difftest")
CHUNK, TIMEOUT = 150, 180


def opt(argv, name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


def run_chunk(jobs, header, data):
    """Runs the exe on these jobs. Returns {addr: result}; a crash or hang is charged to the function that was running."""
    results, todo = {}, list(jobs)
    while todo:
        fd, path = tempfile.mkstemp(prefix="difftest_", suffix=".txt")
        with os.fdopen(fd, "w") as f:
            f.write(header + "".join(f"job {j['addr']:08X} {j['sig']} {j['cases']}\n" for j in todo))
        hang = False
        try:
            p = subprocess.run([EXE, "--difftest", path, "--no-audio", "--no-ini", "--no-mods"] + data, cwd=ROOT,
                               capture_output=True, text=True, errors="replace", timeout=TIMEOUT)
            out, err = p.stdout, p.stderr
        except subprocess.TimeoutExpired as e:
            hang = True
            out = e.stdout.decode(errors="replace") if isinstance(e.stdout, bytes) else (e.stdout or "")
            err = e.stderr.decode(errors="replace") if isinstance(e.stderr, bytes) else (e.stderr or "")
        finally:
            os.remove(path)
        running, case, detail, ended = None, None, [], False
        for line in out.split("\n"):
            w = line.split()
            if not w:
                continue
            if w[0] == "DTRUN":
                running, case, detail = int(w[1], 16), None, []
            elif w[0] == "DTC":
                case = int(w[1])
            elif w[0] == "DTM":
                detail.append(line[4:])
            elif w[0] == "DT" and len(w) >= 4:
                addr = int(w[1], 16)
                nums = {k: int(v) for k, v in (x.split("=") for x in w[4:7] if "=" in x)}
                results[addr] = {"status": w[3], "why": line.split("|", 1)[1].strip() if "|" in line else "", "detail": detail, **nums}
                running = None
            elif w[0] == "DTEND":
                ended = True
        if ended and running is None:
            break
        if running is None:                                    # died outside a function: nothing more to learn
            tail = [x for x in err.strip().split("\n") if x][-3:]
            for j in todo:
                results.setdefault(j["addr"], {"status": "ERROR", "why": " / ".join(tail), "detail": []})
            break
        tail = [x.replace("[port] ", "") for x in err.strip().split("\n") if "crash" in x or "STOP" in x or "stack" in x][:3]
        results[running] = {"status": "HANG" if hang else "CRASH", "detail": detail,
                            "why": (f"the C++ did not come back within {TIMEOUT} s" if hang else " / ".join(tail)) + (f" (case {case})" if case is not None else " (in the original)")}
        todo = [j for j in todo if j["addr"] not in results]
    return results


def main():
    argv = sys.argv[1:]
    if not argv or "-h" in argv or "--help" in argv:
        print(__doc__)
        return 0
    if "--no-build" not in argv:
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "build_pc.py")], cwd=ROOT, capture_output=True, text=True)
        if r.returncode:
            print("difftest: build failed\n" + (r.stdout + r.stderr)[-1500:])
            return 1
    funcs = sigs.scan()
    aud = sigs.audited()
    seed, cases = int(opt(argv, "--seed", "1")), int(opt(argv, "--cases", "40"))
    data = ["--iso", opt(argv, "--iso")] if "--iso" in argv else []
    header = f"seed {seed}\ncalls {1 if '--calls' in argv else 0}\nglobals {1 if '--globals' in argv else 0}\n"
    one = opt(argv, "--fn")
    if one:
        key = one.lower()
        pick = [f for f in funcs if f["name"].lower() == key or f"0x{f['addr']:08x}" == key or f"{f['addr']:08x}" == key]
        if not pick:
            print(f"difftest: no marked definition of {one}")
            return 1
        header += "verbose 1\n" if "--case" in argv or "-v" in argv else ""
        if "--case" in argv:
            header += f"only {int(opt(argv, '--case'))}\n"
    else:
        pick = funcs
        if "--files" in argv:
            keys = opt(argv, "--files").split(",")
            pick = [f for f in pick if any(k in f["file"] for k in keys)]
        if "--no-leaf" in argv:
            pick = [f for f in pick if not f["generated"]]
        if "--audited" in argv:
            pick = [f for f in pick if f["addr"] in aud]
        if "--unaudited" in argv:
            pick = [f for f in pick if f["addr"] not in aud]
        if "--batch" in argv:
            n = max(1, int(opt(argv, "--batch")))
            usable = [f for f in pick if f["sig"]]
            step = max(1, len(usable) // n)
            pick = usable[(seed - 1) % step::step][:n]
        elif "--all" not in argv:
            print(__doc__)
            return 1

    results = {}
    jobs = []
    for f in pick:
        if not f["sig"]:
            results[f["addr"]] = {"status": "UNSUPPORTED", "why": f["reason"], "detail": []}
        else:
            jobs.append({"addr": f["addr"], "sig": f["sig"], "cases": cases})
    for i in range(0, len(jobs), CHUNK):
        results.update(run_chunk(jobs[i:i + CHUNK], header, data))
        if len(jobs) > CHUNK:
            print(f"  {min(i + CHUNK, len(jobs))}/{len(jobs)}", file=sys.stderr, flush=True)

    # Report.
    order = ["MATCH", "KNOWN", "MISMATCH", "CRASH", "HANG", "UNTESTED", "UNSUPPORTED", "NOIMPL", "ERROR"]
    known = {}
    for l in open(os.path.join(ROOT, "tools", "difftest", "known_limits.txt")):
        if l.strip() and not l.startswith("#"):
            a, _, why = l.strip().partition(" ")
            known[int(a, 16)] = why
    rows, lines = [], []
    count = {s: [0, 0] for s in order}
    for f in pick:
        r = results.get(f["addr"], {"status": "ERROR", "why": "no result", "detail": []})
        if r["status"] in ("MISMATCH", "CRASH", "HANG") and f["addr"] in known and "--strict" not in argv:
            r = dict(r, status="KNOWN", why=known[f["addr"]])
        count.setdefault(r["status"], [0, 0])[0 if f["addr"] in aud else 1] += 1
        rows.append({"addr": f"0x{f['addr']:08X}", "name": f["name"], "file": f["file"], "sig": f["sig"], "audited": f["addr"] in aud, **r})
    bad = [r for r in rows if r["status"] in ("MISMATCH", "CRASH", "HANG") or (r["status"] == "KNOWN" and "--show-known" in argv)]
    for r in bad:
        lines.append(f"{r['status']} {r['name']} ({r['addr']}, {r['file']}{', audited' if r['audited'] else ''}): {r.get('why', '')}")
        lines += ["    " + d for d in r["detail"]]
        m = [d for d in r["detail"] if " case " in d and "seed" in d]
        if m:
            lines.append(f"    reproduce: python tools/difftest.py --fn {r['addr']} --seed {seed} --case {m[0].split(' case ')[1].split()[0]}" + (" --calls" if "--calls" in argv else ""))
    tested = sum(sum(count[s]) for s in ("MATCH", "MISMATCH", "CRASH", "HANG"))
    lines.append(f"difftest: {len(rows)} functions, {tested} tested: " + ", ".join(f"{sum(count[s])} {s.lower()}" for s in order if sum(count.get(s, [0, 0]))))
    lines.append("  audited:   " + ", ".join(f"{count[s][0]} {s.lower()}" for s in order if count[s][0]))
    lines.append("  unaudited: " + ", ".join(f"{count[s][1]} {s.lower()}" for s in order if count[s][1]))
    if one:
        r = rows[0]
        lines.insert(0, f"{r['status']} {r['name']} ({r['addr']}, sig {r['sig']}): ok={r.get('ok', 0)} skipped={r.get('skipped', 0)} mismatched={r.get('mismatched', 0)} {r.get('why', '')}")
        if r["status"] not in ("MISMATCH", "CRASH", "HANG"):
            lines[1:1] = ["    " + d for d in r["detail"]]
        lines = lines[:-3]
    else:
        os.makedirs(OUT, exist_ok=True)
        json.dump({"seed": seed, "cases": cases, "calls": "--calls" in argv, "functions": rows}, open(os.path.join(OUT, "report.json"), "w"), indent=1)
        why = {}
        for r in rows:
            if r["status"] in ("UNSUPPORTED", "UNTESTED"):
                k = r["status"] + ": " + " ".join(("fn" if x.startswith(("fn_", "0x")) or x[:1].isupper() and len(x) > 3 else x) for x in r.get("why", "").split())[:60]
                why[k] = why.get(k, 0) + 1
        top = sorted(why.items(), key=lambda kv: -kv[1])[:12]
        open(os.path.join(OUT, "report.txt"), "w").write("\n".join(lines + ["", "not tested, most common reasons:"] + [f"  {n:5d}  {k}" for k, n in top]) + "\n")
        lines.append("  report: build/difftest/report.txt, report.json")
    print("\n".join(lines))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
