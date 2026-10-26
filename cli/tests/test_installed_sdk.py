#!/usr/bin/env python3
"""Installed SDK admission: minimum stable version and compiled public macros."""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as directory:
    prefix = pathlib.Path(directory)
    headers = prefix / "include/maelys"
    headers.mkdir(parents=True)
    (prefix / "lib/pkgconfig").mkdir(parents=True)
    (prefix / "lib/libmaelys_datalog.a").write_bytes(b"")
    for name in ("datalog_resources.h", "datalog_explanations.h"):
        (headers / name).touch()
    pin = prefix / "minimum.pin"
    pin.write_text("v0.26.0\n" + "0" * 40 + "\n")
    cases = [
        ("0.26.0", "2u", "2u", None),
        ("0.27.0", "2u", "2u", None),
        ("0.100.0", "(1 + 1)", "(2)", None),
        ("0.25.9", "2u", "2u", "older than minimum 0.26.0"),
        ("0.27.0", "3u", "2u", "public API 2"),
        ("0.27.0", "2u", "3u", "program ABI 2"),
        ("0.27.0-rc.1", "2u", "2u", "stable X.Y.Z"),
        ("0.27.0", None, "2u", "public API 2"),
    ]
    for release, api, abi, rejection in cases:
        (prefix / "lib/pkgconfig/maelys-datalog.pc").write_text(f"Version: {release}\n")
        (headers / "datalog.h").write_text(
            f"#define MAELYS_DATALOG_PUBLIC_API_VERSION {api}\n" if api else "")
        (headers / "datalog_program.h").write_text(f"#define MAELYS_DATALOG_PROGRAM_ABI_VERSION {abi}\n")
        before = {p.relative_to(prefix): p.read_bytes() for p in prefix.rglob("*") if p.is_file()}
        result = subprocess.run([sys.executable, str(root / "scripts/check-installed-sdk.py"),
            str(prefix), str(pin)], text=True, capture_output=True)
        assert (result.returncode == 0) == (rejection is None), result.stderr
        if rejection:
            assert "Installed SDK rejected:" in result.stderr and rejection in result.stderr, result.stderr
        assert before == {p.relative_to(prefix): p.read_bytes() for p in prefix.rglob("*") if p.is_file()}
print(f"installed SDK admission: {len(cases)} cases PASS (prefix unchanged)")
