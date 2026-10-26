#!/usr/bin/env python3
"""The installed manual must render the committed catalog of this version."""
import pathlib
import subprocess
import sys

tool, binary, manual, date = sys.argv[1:]
root = pathlib.Path(__file__).resolve().parents[2]
version = (root / "VERSION").read_text().strip()
expected = subprocess.check_output([sys.executable, tool, "--date", date,
    "--catalog", str(root / "docs/cli-contract.json"), "--version", version, "maelys-datalog"])
assert pathlib.Path(manual).read_bytes() == expected, "manual differs from committed catalog"
mdx = subprocess.check_output([sys.executable, tool, "--date", date, "--format", "mdx",
    "--body-only", binary], text=True)
assert "maelys-datalog" in mdx and "--why" in mdx and "--manifest" in mdx
print("manual and archived catalog: PASS")

fr_native = subprocess.check_output([sys.executable, tool, "--date", date, "--lang", "fr", binary])
fr_catalog = subprocess.check_output([sys.executable, tool, "--date", date, "--lang", "fr",
    "--catalog", str(root / "docs/cli-contract.json"), "--version", version, "maelys-datalog"])
assert fr_native == fr_catalog
assert not subprocess.check_output([sys.executable, tool, "--lang", "fr", "--untranslated", binary]).strip()
print("French manual and complete catalog translation: PASS")
