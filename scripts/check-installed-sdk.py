#!/usr/bin/env python3
"""Admit compatible installed SDKs without writing to their prefix."""
import os
import pathlib
import re
import shlex
import subprocess
import sys


def version(text):
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", text):
        raise ValueError(f"expected a stable X.Y.Z version, found {text!r}")
    return tuple(map(int, text.split(".")))


prefix = pathlib.Path(sys.argv[1])
expected = pathlib.Path(sys.argv[2]).read_text().splitlines()[0].removeprefix("v")
try:
    metadata = (prefix / "lib/pkgconfig/maelys-datalog.pc").read_text()
    versions = [line.split(":", 1)[1].strip() for line in metadata.splitlines()
                if line.startswith("Version:")]
    if len(versions) != 1:
        raise ValueError(f"expected one SDK Version field, found {versions}")
    if version(versions[0]) < version(expected):
        raise ValueError(f"SDK version {versions[0]} is older than minimum {expected}")
    for name in ("include/maelys/datalog.h", "include/maelys/datalog_resources.h",
                 "include/maelys/datalog_program.h", "include/maelys/datalog_explanations.h",
                 "lib/libmaelys_datalog.a"):
        if not (prefix / name).is_file():
            raise ValueError(f"missing {name}")
    # Compile the public contract, not header text or a private layout. This
    # handles conditional definitions and macro expressions as the consumer does.
    source = """#include <maelys/datalog.h>
#include <maelys/datalog_program.h>
#if !defined(MAELYS_DATALOG_PUBLIC_API_VERSION) || MAELYS_DATALOG_PUBLIC_API_VERSION != 2
#error installed SDK must provide public API 2
#endif
#if !defined(MAELYS_DATALOG_PROGRAM_ABI_VERSION) || MAELYS_DATALOG_PROGRAM_ABI_VERSION != 2
#error installed SDK must provide program ABI 2
#endif
"""
    checked = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=c11",
        "-fsyntax-only", "-x", "c", "-I", str(prefix / "include"), "-"],
        input=source, text=True, capture_output=True, timeout=30)
    if checked.returncode:
        raise ValueError("public SDK contract must be API 2 / program ABI 2:\n" + checked.stderr.strip())
except (OSError, ValueError, subprocess.TimeoutExpired) as error:
    sys.exit(f"Installed SDK rejected: {error}")
