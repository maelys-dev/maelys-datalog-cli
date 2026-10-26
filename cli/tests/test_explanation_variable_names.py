#!/usr/bin/env python3
"""Golden source-name/opaque-ID cases, run against a real CLI executable."""
import argparse
import difflib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / 'cli/tests/fixtures/variable-names'
CASES = {
    'long': ('allow(User, Resource) :- right(User, Resource).',
             'can_deliver(User) :- allow(User, "billing:read"), request(User, "billing:read").'),
    'reversed': ('allow(Resource, User) :- right(User, Resource).',
                 'can_deliver(User) :- allow("billing:read", User), request(User, "billing:read").'),
    'mixed': ('allow(U, Resource) :- right(U, Resource).',
              'can_deliver(U) :- allow(U, "billing:read"), request(U, "billing:read").'),
    'anonymous': ('allow(User, "billing:read") :- right(User, _).',
                  'can_deliver(User) :- allow(User, "billing:read"), request(User, "billing:read").'),
    'anonymous-bound': ('allow("Mallory", "billing:read") :- request(_, "billing:read"), right("Mallory", "billing:read").',
                        'can_deliver(User) :- allow(User, "billing:read"), request(User, "billing:read").'),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('--source-names', action='store_true',
                        help='expect the SDK source-name API path')
    args = parser.parse_args()
    failed = 0
    with tempfile.TemporaryDirectory(prefix='datalog-variable-names-') as tmp:
        tmp = Path(tmp)
        domain = tmp / 'domain.json'
        domain.write_text(json.dumps({'format': 'maelys-datalog-domain-v1', 'name': 'variable_names',
            'predicates': [
                {'name': 'right', 'arity': 2, 'role': 'policy_fact'},
                {'name': 'request', 'arity': 2, 'role': 'edb'},
                {'name': 'allow', 'arity': 2, 'role': 'idb'},
                {'name': 'can_deliver', 'arity': 1, 'role': 'idb', 'query': True}],
            'atoms': ['Leela', 'Mallory', 'billing:read']}))
        facts = tmp / 'facts.dl'
        facts.write_text('request("Mallory", "billing:read").\n')
        for case, rules in CASES.items():
            policy = tmp / f'{case}.dl'
            policy.write_text('right("Leela", "billing:read").\n' + '\n'.join(rules) + '\n')
            command = [str(args.binary.resolve()), 'explain', '--domain', str(domain),
                       '--facts', str(facts), '--why', 'false', str(policy),
                       'can_deliver', '"Mallory"']
            for lang in ('en', 'fr'):
                environment = dict(os.environ, MAELYS_CLI_LANG=lang,
                                   DATALOG_CLI_TEST_NAMES_CASE=case)
                result = subprocess.run(command + ['--format', 'text'], env=environment, capture_output=True, text=True)
                suffix = 'names' if args.source_names else 'ids'
                golden = FIXTURES / f'{case}-{lang}-{suffix}.txt'
                expected = golden.read_text()
                if result.returncode or result.stderr or result.stdout != expected:
                    failed += 1
                    print(f'FAIL {case}/{lang} ({suffix}): exit={result.returncode}')
                    print(''.join(difflib.unified_diff(expected.splitlines(True),
                        result.stdout.splitlines(True), fromfile=str(golden.name),
                        tofile='actual')), end='')
                    if result.stderr:
                        print(result.stderr, end='')
                else:
                    print(f'PASS {case}/{lang} ({suffix})')
                # Machine records retain the SDK variable IDs and raw document.
                structured = subprocess.run(command + ['--format', 'json', '--compact'],
                    env=environment, capture_output=True, text=True, check=True)
                data = json.loads(structured.stdout)['data']
                raw = subprocess.run(command + ['--raw', '--format', 'text'], env=environment,
                    capture_output=True, text=True, check=True)
                assert raw.stdout == data['document']
                assert all(isinstance(b['variable'], int) for o in data['structure']['obstacles']
                           for b in o['bindings'])
    print(f'variable-name goldens: {2*len(CASES)-failed} passed, {failed} failed')
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
