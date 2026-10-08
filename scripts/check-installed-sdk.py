#!/usr/bin/env python3
"""Check an explicitly selected installed SDK without modifying its prefix."""
import pathlib
import sys
prefix = pathlib.Path(sys.argv[1])
expected = pathlib.Path(sys.argv[2]).read_text().splitlines()[0].removeprefix('v')
try:
    metadata = (prefix / 'lib/pkgconfig/maelys-datalog.pc').read_text()
    versions = [line.split(':', 1)[1].strip() for line in metadata.splitlines()
                if line.startswith('Version:')]
    if versions != [expected]:
        raise ValueError(f'expected pinned SDK version {expected}, found {versions}')
    for name in ('include/maelys/datalog.h', 'include/maelys/datalog_resources.h',
                 'include/maelys/datalog_program.h', 'include/maelys/datalog_explanations.h',
                 'lib/libmaelys_datalog.a'):
        if not (prefix / name).is_file():
            raise ValueError(f'missing {name}')
except (OSError, ValueError) as error:
    sys.exit(f'Installed SDK rejected: {error}')
