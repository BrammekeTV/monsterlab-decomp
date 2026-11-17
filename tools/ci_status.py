"""Wait for the GitHub Actions runs of a commit and print results + error annotations.

    python tools/ci_status.py [sha] [--timeout SECONDS]

Uses the public GitHub API (no token needed for this public repo).
"""
import json
import subprocess
import sys
import time
import urllib.request

REPO = "BrammekeTV/monsterlab-decomp"
API = f"https://api.github.com/repos/{REPO}"


def get(url):
    req = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json",
                                               "User-Agent": "ci-status-script"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    timeout = 600
    if "--timeout" in sys.argv:
        timeout = int(sys.argv[sys.argv.index("--timeout") + 1])
        args = [a for a in args if a != str(timeout)]
    sha = args[0] if args else subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()

    deadline = time.time() + timeout
    while True:
        runs = [r for r in get(f"{API}/actions/runs?per_page=20")["workflow_runs"] if r["head_sha"].startswith(sha)]
        if runs and all(r["status"] == "completed" for r in runs):
            break
        if time.time() > deadline:
            print("timeout; runs:", [(r["name"], r["status"]) for r in runs])
            return 2
        time.sleep(15)

    failed = False
    for r in runs:
        print(f"{r['name']}: {r['conclusion']}")
        if r["conclusion"] != "success":
            failed = True
            for job in get(r["jobs_url"])["jobs"]:
                for a in get(f"{API}/check-runs/{job['id']}/annotations"):
                    if a["annotation_level"] == "failure":
                        loc = f"{a['path']}:{a['start_line']}" if a.get("path") and a["path"] != ".github" else ""
                        print(f"  {loc} {a['message']}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
