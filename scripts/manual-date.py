#!/usr/bin/env python3
"""Use the latest dated release entry even in a source archive without git."""
import datetime
import pathlib
import re
text = (pathlib.Path(__file__).resolve().parents[1] / "CHANGELOG.md").read_text()
match = re.search(r"^## [0-9]+\.[0-9]+\.[0-9]+ [—-] (\d{4}-\d{2}-\d{2})$", text, re.M)
if not match:
    raise SystemExit("No dated release entry for the manual")
print(datetime.date.fromisoformat(match[1]).isoformat())
