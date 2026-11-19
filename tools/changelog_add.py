"""Add a changelog entry to docs/data/features.json stamped with the real
current Amsterdam time (CET/CEST). Usage: python tools/changelog_add.py "text"
Never type a date or time by hand."""
import datetime, json, sys, zoneinfo
from pathlib import Path

P = Path(__file__).resolve().parent.parent / "docs/data/features.json"
now = datetime.datetime.now(zoneinfo.ZoneInfo("Europe/Amsterdam"))
d = json.loads(P.read_text(encoding="utf-8"))
d["changelog"].insert(0, {"date": now.strftime("%Y-%m-%d"), "time": now.strftime("%H:%M"), "text": sys.argv[1]})
P.write_text(json.dumps(d, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
print("added", now.strftime("%Y-%m-%d %H:%M"))
