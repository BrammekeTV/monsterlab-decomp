"""Wait for the GitHub Actions runs of a commit and print results + error annotations.

    python tools/ci_status.py [sha] [--timeout SECONDS]

Uses the GitHub CLI (gh) when installed and logged in; otherwise the anonymous API
(60 requests/hour, or GITHUB_TOKEN if set).
"""
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request

REPO = "BrammekeTV/monsterlab-decomp"
API = f"https://api.github.com/repos/{REPO}"


def _gh_get(url):
    """Use the GitHub CLI when it is installed and logged in (gh auth login): 5000 requests/hour."""
    import shutil
    gh = shutil.which("gh")
    default = r"C:\Program Files\GitHub CLI\gh.exe"
    if gh is None and os.path.exists(default):
        gh = default
    if gh is None:
        return None
    path = url.replace("https://api.github.com/", "")
    r = subprocess.run([gh, "api", path], capture_output=True, text=True, encoding="utf-8")
    if r.returncode != 0:
        return None
    return json.loads(r.stdout)


def get(url):
    """GET a GitHub API url: via gh if available, else anonymously (60 requests/hour; on
    403/429 wait for the reset). GITHUB_TOKEN is used for anonymous calls when set."""
    data = _gh_get(url)
    if data is not None:
        return data
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "ci-status-script"}
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    req = urllib.request.Request(url, headers=headers)
    while True:
        try:
            with urllib.request.urlopen(req, timeout=30) as r:
                return json.load(r)
        except urllib.error.HTTPError as e:
            if e.code not in (403, 429):
                raise
            reset = int(e.headers.get("X-RateLimit-Reset", time.time() + 60))
            wait = max(10, min(900, reset - time.time() + 2))
            print(f"rate limited; waiting {int(wait)} s", flush=True)
            time.sleep(wait)


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
        time.sleep(60)

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
