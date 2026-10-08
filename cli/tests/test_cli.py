#!/usr/bin/env python3
"""Product behavior and data schemas; the spec kit covers the shared CLI."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "cli/tests/fixtures"
BINARY = pathlib.Path(sys.argv[1])
SPEC = pathlib.Path(sys.argv[2])
sys.path.insert(0, str(SPEC / "conformance"))
from validate import validate  # noqa: E402


def run(command, *arguments, exit_code=0):
    process = subprocess.run(
        [str(BINARY), command, *map(str, arguments), "--format", "json", "--compact"],
        capture_output=True, text=True, cwd=ROOT, check=False,
    )
    assert process.returncode == exit_code, (command, process.stdout, process.stderr)
    payload = process.stderr if exit_code == 1 else process.stdout
    assert not (process.stdout if exit_code == 1 else process.stderr), process
    result = json.loads(payload)
    envelope = json.loads((SPEC / "schemas/envelope.json").read_text())
    assert not validate(result, envelope), validate(result, envelope)
    assert result["command"] == command
    assert result["exitCode"] == exit_code
    assert result["ok"] == (exit_code != 1)
    if exit_code != 1:
        schema = json.loads((ROOT / f"cli/schemas/{command}.json").read_text())
        assert not validate(result["data"], schema), validate(result["data"], schema)
    return result.get("data", result.get("error"))


def text(command, *arguments, exit_code=0):
    process = subprocess.run([str(BINARY), command, *map(str, arguments)],
                             capture_output=True, text=True, cwd=ROOT, check=False)
    assert process.returncode == exit_code, (process.stdout, process.stderr)
    if exit_code == 1:
        assert process.stderr.startswith("maelys-datalog: ["), process.stderr
        assert not process.stdout
    else:
        assert process.stdout and not process.stderr, process
    return process.stderr if exit_code == 1 else process.stdout


def help_catalog():
    catalog = json.loads(subprocess.check_output(
        [str(BINARY), "describe", "--format", "json"], text=True))["data"]
    overview = text("help")
    for command in catalog["commands"]:
        if command["id"] not in {"check", "queries", "fingerprint", "solve", "explain"}:
            continue
        # Match content, not alignment, wrapping, capitalization or line count.
        assert " ".join(command["pattern"]) in overview
        assert " ".join(command["purpose"].split()) in " ".join(overview.split())
        page = text("help", command["id"])
        assert " ".join(command["pattern"]) in page
        assert " ".join(command["purpose"].split()) in " ".join(page.split())
        assert " ".join(command["usage"].split()) in " ".join(page.split())
        assert command["examples"]
        for example in command["examples"]:
            assert " ".join(example["words"]) in " ".join(page.split())


def temporary_file(directory, name, content):
    path = pathlib.Path(directory) / name
    path.write_bytes(content if isinstance(content, bytes) else content.encode())
    return path


def basic():
    domain = FIXTURE / "rbac.domain.json"
    policy = FIXTURE / "rbac.dl"
    facts = FIXTURE / "rbac.facts.dl"
    checked = run("check", "--domain", domain, policy)
    assert checked["valid"] and checked["normalizedRuleCount"] == 2
    assert checked["policyCount"] == 1
    assert checked["policies"][0]["policyId"] == "cli"
    assert checked["domains"][0]["predicates"][3]["query"] is True
    queries = run("queries", "--domain", domain, policy)
    assert queries["policyFingerprint"] == checked["policyFingerprint"]
    assert queries["policies"] == [{
        "index": 0, "policyId": "cli", "domain": "rbac_cli", "queries": [
            {"predicate": "allow", "arity": 2},
            {"predicate": "can_deliver", "arity": 1},
        ],
    }]
    named = run("check", "--domain", domain, "--policy-id", "named", policy)
    assert named["policies"][0]["policyId"] == "named"
    assert run("queries", "--domain", domain, "--policy-id", "named", policy)[
        "policies"][0]["policyId"] == "named"
    identity = run("fingerprint", "--domain", domain, policy)
    assert identity["policyFingerprint"] == checked["policyFingerprint"]
    assert text("fingerprint", "--domain", domain, policy) == (
        f'policyFingerprint: {identity["policyFingerprint"]}\n'
        f'executionFingerprint: {identity["executionFingerprint"]}\n')
    solved = run("solve", "--domain", domain, "--facts", facts, policy)
    assert solved["derivedFactCount"] == 2
    assert solved["policyFingerprint"] == identity["policyFingerprint"]
    assert solved["executionFingerprint"] == identity["executionFingerprint"]
    assert [q["predicate"] for q in solved["queries"]] == ["allow", "can_deliver"]
    assert solved["queries"][1]["facts"] == [[{"kind": "symbol", "value": "Leela"}]]
    selected = run("solve", "--domain", domain, "--facts", facts,
                   "--query", "can_deliver/1", policy)
    assert len(selected["queries"]) == 1
    assert text("solve", "--domain", domain, "--facts", facts,
                "--query", "can_deliver/1", policy) == (
                    'can_deliver/1 (1 facts):\n  can_deliver("Leela").\n')
    yes = run("explain", "--domain", domain, "--facts", facts,
              "--why", "true", policy, "can_deliver", '"Leela"')
    no = run("explain", "--domain", domain, "--facts", facts,
             "--why", "false", policy, "can_deliver", '"Mallory"')
    assert yes["present"] and "status=complete" in yes["document"]
    assert not no["present"] and "status=complete" in no["document"]
    assert "document=why-false" in no["document"]
    for command, args in (
        ("check", ("--domain", domain, policy)),
        ("queries", ("--domain", domain, policy)),
        ("fingerprint", ("--domain", domain, policy)),
        ("solve", ("--domain", domain, "--facts", facts, policy)),
        ("explain", ("--domain", domain, "--facts", facts,
                     "--why", "false", policy, "can_deliver", '"Mallory"')),
    ):
        output = text(command, *args)
        assert re.sub(r"[0-9a-f]{64}", "<fingerprint>", output) == (
            FIXTURE / f"golden-{command}.txt").read_text()
        result = run(command, *args)
        for key in ("policyFingerprint", "executionFingerprint"):
            if key in result:
                result[key] = "<fingerprint>"
        assert result == json.loads((FIXTURE / f"golden-{command}.json").read_text())
    assert run("solve", "--domain", domain, "--facts", facts,
               "--query", "missing", policy, exit_code=1)["code"] == "NOT_FOUND"
    assert run("explain", "--domain", domain, "--facts", facts,
               "--why", "false", policy, "can_deliver", exit_code=1)["code"] == "VALIDATION_FAILED"
    work_limit_message = (
        "--work-limit requires WORK_LIMIT capability; the reference backend "
        "does not support a nonzero work limit")
    for command, args in (
        ("fingerprint", ("--domain", domain, policy)),
        ("solve", ("--domain", domain, "--facts", facts, policy)),
        ("explain", ("--domain", domain, "--facts", facts,
                     "--why", "true", policy, "can_deliver", '"Leela"')),
    ):
        error = run(command, *args, "--work-limit", "1", exit_code=1)
        assert error["code"] == "UNSUPPORTED"
        assert error["message"] == work_limit_message
        assert work_limit_message in text(command, *args, "--work-limit", "1", exit_code=1)
    assert run("check", "--domain", domain, "--policy-id", "", policy,
               exit_code=1)["code"] == "VALIDATION_FAILED"
    version = subprocess.run([str(BINARY), "--version"], capture_output=True,
                             text=True, check=False)
    assert version.returncode == 0 and (ROOT / "VERSION").read_text().strip() in version.stdout


def malformed():
    domain = FIXTURE / "rbac.domain.json"
    policy = FIXTURE / "rbac.dl"
    with tempfile.TemporaryDirectory() as temp:
        bad_policy = temporary_file(temp, "bad.dl", "allow(X) :- missing(X).\n")
        report = run("check", "--domain", domain, bad_policy, exit_code=2)
        assert not report["valid"] and report["diagnostics"][0]["line"] == 1
        text("check", "--domain", domain, bad_policy, exit_code=2)
        assert run("queries", "--domain", domain, bad_policy,
                   exit_code=1)["code"] == "VALIDATION_FAILED"

        bad_domain = json.loads(domain.read_text())
        bad_domain["unknown"] = 1
        path = temporary_file(temp, "unknown.json", json.dumps(bad_domain))
        report = run("check", "--domain", path, policy, exit_code=2)
        assert report["diagnostics"][0]["code"] == "DOMAIN_INVALID"
        assert run("solve", "--domain", path, "--facts",
                   FIXTURE / "rbac.facts.dl", policy, exit_code=1)["code"] == "VALIDATION_FAILED"
        assert run("queries", "--domain", path, policy,
                   exit_code=1)["code"] == "VALIDATION_FAILED"
        for name, content in (
            ("truncated.json", b'{"format":'),
            ("duplicate.json", b'{"format":"x","format":"y"}'),
            ("nul.json", b'{"name":"\\u0000"}'),
            ("utf8.json", b'{"name":"\xff"}'),
        ):
            path = temporary_file(temp, name, content)
            assert run("check", "--domain", path, policy, exit_code=1)["code"] == "VALIDATION_FAILED"
        bad_domain = json.loads(domain.read_text())
        bad_domain["predicates"].append(bad_domain["predicates"][0])
        path = temporary_file(temp, "duplicate-predicate.json", json.dumps(bad_domain))
        assert run("check", "--domain", path, policy, exit_code=2)["diagnostics"][0]["index"] == 5
        bad_domain = json.loads(domain.read_text())
        bad_domain["atoms"].append("admin")
        path = temporary_file(temp, "duplicate-atom.json", json.dumps(bad_domain))
        assert run("check", "--domain", path, policy, exit_code=2)["diagnostics"][0]["index"] == 3
        for name, content in (
            ("unterminated.dl", 'request("Leela).'),
            ("overflow.dl", "request(9223372036854775808, true)."),
            ("arity.dl", 'request("a", "b", "c", "d", "e").'),
            ("utf8.dl", b'request("\xff", "b").'),
            ("nul.dl", b'request("a", "b").\x00'),
        ):
            path = temporary_file(temp, name, content)
            error = run("solve", "--domain", domain, "--facts", path,
                        policy, exit_code=1)
            assert error["code"] == "VALIDATION_FAILED"
        missing = pathlib.Path(temp) / "missing.json"
        for command, args in (
            ("check", ("--domain", missing, policy)),
            ("solve", ("--domain", missing, "--facts",
                       FIXTURE / "rbac.facts.dl", policy)),
        ):
            error = run(command, *args, exit_code=1)
            assert error["code"] == "NOT_FOUND"
            assert error["message"] == f"domain file not found: {missing}"
        for command, args, message in (
            ("check", ("--domain", domain, missing), "policy file not found"),
            ("fingerprint", ("--domain", domain, missing), "policy file not found"),
            ("solve", ("--domain", domain, "--facts", missing, policy),
             "facts file not found"),
            ("check", ("--domain", domain, "--manifest", missing),
             "manifest file not found"),
            ("queries", ("--domain", domain, "--manifest", missing),
             "manifest file not found"),
        ):
            error = run(command, *args, exit_code=1)
            assert error["code"] == "NOT_FOUND"
            assert error["message"] == f"{message}: {missing}"
        blank = temporary_file(temp, "blank.dl", "\n\nrequest(\"Leela\", \"billing:read\").\n")
        assert run("solve", "--domain", domain, "--facts", blank,
                   policy)["derivedFactCount"] == 2


def escaped_symbols():
    with tempfile.TemporaryDirectory() as temp:
        domain = temporary_file(temp, "domain.json", json.dumps({
            "format": "maelys-datalog-domain-v1", "name": "escaped_cli",
            "predicates": [
                {"name": "input", "arity": 1, "role": "edb"},
                {"name": "output", "arity": 1, "role": "idb", "query": True},
            ], "atoms": [],
        }))
        policy = temporary_file(temp, "policy.dl", "output(X) :- input(X).\n")
        facts = temporary_file(temp, "facts.dl", 'input("quote\\" slash\\\\ alpha\\u03b1").')
        expected = 'quote" slash\\ alphaα'
        result = run("solve", "--domain", domain, "--facts", facts, policy)
        assert result["queries"][0]["facts"] == [[{"kind": "symbol", "value": expected}]]
        human = text("solve", "--domain", domain, "--facts", facts, policy)
        assert human == 'output/1 (1 facts):\n  output("quote\\" slash\\\\ alphaα").\n'
        assert run("explain", "--domain", domain, "--facts", facts,
                   "--why", "true", policy, "output",
                   '"quote\\" slash\\\\ alpha\\u03b1"')["present"]
        controls = temporary_file(temp, "controls.dl",
                                  'input("line\\u000afeed \\uD83D\\uDE00").')
        control_result = run("solve", "--domain", domain, "--facts", controls, policy)
        assert control_result["queries"][0]["facts"][0][0]["value"] == "line\nfeed 😀"
        assert 'output("line\\u000afeed 😀").' in text(
            "solve", "--domain", domain, "--facts", controls, policy)
        for name, literal in (("bad-escape", '"bad\\x"'),
                              ("bad-surrogate", '"bad\\uD800"'),
                              ("nul", '"bad\\u0000"')):
            invalid = temporary_file(temp, f"{name}.dl", f"input({literal}).")
            assert run("solve", "--domain", domain, "--facts", invalid,
                       policy, exit_code=1)["code"] == "VALIDATION_FAILED"


def schema_arity():
    schema = json.loads((ROOT / "cli/schemas/solve.json").read_text())
    valid = run("solve", "--domain", FIXTURE / "rbac.domain.json", "--facts",
                FIXTURE / "rbac.facts.dl", FIXTURE / "rbac.dl")
    assert not validate(valid, schema)
    invalid = json.loads(json.dumps(valid))
    invalid["queries"][0]["facts"][0].pop()
    assert validate(invalid, schema)
    invalid = json.loads(json.dumps(valid))
    invalid["queries"][0]["facts"][0].extend(invalid["queries"][0]["facts"][0] * 2)
    assert validate(invalid, schema)


def integers_and_manifest():
    with tempfile.TemporaryDirectory() as temp:
        d1 = {
            "format": "maelys-datalog-domain-v1", "name": "numbers_cli",
            "predicates": [
                {"name": "input", "arity": 1, "role": "edb"},
                {"name": "allow", "arity": 1, "role": "idb", "query": True},
            ], "atoms": [],
        }
        domain = temporary_file(temp, "numbers.json", json.dumps(d1))
        source = "allow(X) :- input(X).\n"
        policy = temporary_file(temp, "numbers.dl", source)
        facts = temporary_file(temp, "bounds.dl",
                               "input(-9223372036854775808). input(9223372036854775807).")
        solved = run("solve", "--domain", domain, "--facts", facts, policy)
        assert [row[0]["value"] for row in solved["queries"][0]["facts"]] == [
            -9223372036854775808, 9223372036854775807]
        overflow = temporary_file(temp, "overflow.dl", "input(9223372036854775808).")
        assert "fact 0" in run("solve", "--domain", domain, "--facts", overflow,
                               policy, exit_code=1)["message"]
        d2 = dict(d1, name="numbers_two_cli")
        domain2 = temporary_file(temp, "numbers-two.json", json.dumps(d2))
        source2 = "allow(X) :- input(X).\n"
        policy2 = temporary_file(temp, "numbers-two.dl", source2)
        manifest = {
            "policy_set_id": "numbers", "policy_set_version": "1",
            "manifest_version": "1", "default_profile": "enforce",
            "created_for": "test", "strict_loading": True, "fail_closed": True,
            "capabilities": [], "policies": []
        }
        for index, (name, file, src) in enumerate((
            ("numbers_cli", policy, source), ("numbers_two_cli", policy2, source2))):
            manifest["policies"].append({
                "policy_id": f"policy{index}", "domain": name,
                "file": file.name, "sha256": hashlib.sha256(src.encode()).hexdigest(),
                "mode": "enforce", "enabled": True, "description": "CLI test",
                "queries": [{"name": "allow", "arity": 1}],
            })
        manifest_path = temporary_file(temp, "manifest.json", json.dumps(manifest))
        report = run("check", "--domain", domain, "--domain", domain2,
                     "--manifest", manifest_path)
        assert report["valid"] and report["policyCount"] == 2
        assert [p["policyId"] for p in report["policies"]] == ["policy0", "policy1"]
        queries = run("queries", "--domain", domain, "--domain", domain2,
                      "--manifest", manifest_path)
        assert queries["policies"] == [
            {"index": index, "policyId": f"policy{index}", "domain": name,
             "queries": [{"predicate": "allow", "arity": 1}]}
            for index, name in enumerate(("numbers_cli", "numbers_two_cli"))]
        assert run("check", "--domain", domain, "--domain", domain2,
                   "--manifest", manifest_path, "--policy-id", "x",
                   exit_code=1)["code"] == "VALIDATION_FAILED"

        # A domain query flag does not bypass an absent or empty whitelist.
        manifest["policies"][0]["queries"] = []
        del manifest["policies"][1]["queries"]
        manifest_path.write_text(json.dumps(manifest))
        queries = run("queries", "--domain", domain, "--domain", domain2,
                      "--manifest", manifest_path)
        assert all(not p["queries"] for p in queries["policies"])
        assert text("queries", "--domain", domain, "--domain", domain2,
                    "--manifest", manifest_path).count("no authorized queries") == 2

        # Disabled entries are omitted and enabled-policy indices stay compact.
        manifest["policies"][0]["enabled"] = False
        manifest_path.write_text(json.dumps(manifest))
        checked = run("check", "--domain", domain, "--domain", domain2,
                      "--manifest", manifest_path)
        assert checked["policies"] == [
            {"index": 0, "policyId": "policy1", "normalizedRuleCount": 1}]
        assert run("queries", "--domain", domain, "--domain", domain2,
                   "--manifest", manifest_path)["policies"][0]["policyId"] == "policy1"

        # A successful empty policy handle is a valid result, not a schema error.
        manifest["policies"][1]["enabled"] = False
        manifest_path.write_text(json.dumps(manifest))
        checked = run("check", "--domain", domain, "--domain", domain2,
                      "--manifest", manifest_path)
        assert checked["valid"] and checked["policyCount"] == 0
        fingerprint = checked["policyFingerprint"]
        assert re.fullmatch(r"[0-9a-f]{64}", fingerprint)
        assert checked["policies"] == [] and checked["normalizedRuleCount"] == 0
        invalid_empty = dict(checked, policyFingerprint=None)
        assert validate(invalid_empty, json.loads((ROOT / "cli/schemas/check.json").read_text()))
        empty = run("queries", "--domain", domain, "--domain", domain2,
                    "--manifest", manifest_path)
        assert empty["policies"] == [] and empty["policyFingerprint"] == fingerprint
        assert validate(dict(empty, policyFingerprint=None),
                        json.loads((ROOT / "cli/schemas/queries.json").read_text()))
        assert text("queries", "--domain", domain, "--domain", domain2,
                    "--manifest", manifest_path) == "No enabled policies.\n"
        # Disabled entries and manifest formatting do not change the SDK identity.
        manifest["policies"] = []
        manifest_path.write_text(json.dumps(manifest, indent=2))
        assert run("check", "--domain", domain, "--domain", domain2,
                   "--manifest", manifest_path)["policyFingerprint"] == fingerprint
        assert run("queries", "--domain", domain, "--domain", domain2,
                   "--manifest", manifest_path)["policyFingerprint"] == fingerprint


def manifest_query_whitelist():
    with tempfile.TemporaryDirectory() as temp:
        domain = FIXTURE / "rbac.domain.json"
        source = (FIXTURE / "rbac.dl").read_text()
        temporary_file(temp, "policy.dl", source)
        manifest = {
            "policy_set_id": "rbac", "policy_set_version": "1",
            "manifest_version": "1", "default_profile": "enforce",
            "created_for": "test", "strict_loading": True, "fail_closed": True,
            "capabilities": [], "policies": [{
                "policy_id": "restricted", "domain": "rbac_cli",
                "file": "policy.dl", "sha256": hashlib.sha256(source.encode()).hexdigest(),
                "mode": "enforce", "enabled": True, "description": "whitelist",
                "queries": [{"name": "can_deliver", "arity": 1}],
            }],
        }
        path = temporary_file(temp, "manifest.json", json.dumps(manifest))
        result = run("queries", "--domain", domain, "--manifest", path)
        assert result["policies"][0]["queries"] == [
            {"predicate": "can_deliver", "arity": 1}]
        invalid = dict(result, policyFingerprint=None)
        assert validate(invalid, json.loads((ROOT / "cli/schemas/queries.json").read_text()))
        assert text("queries", "--domain", domain, "--manifest", path) == (
            'policy "restricted" (domain rbac_cli):\n  can_deliver/1\n')
        assert run("queries", "--domain", domain, "--manifest", path,
                   "--policy-id", "other", exit_code=1)["code"] == "VALIDATION_FAILED"


if __name__ == "__main__":
    basic()
    malformed()
    escaped_symbols()
    schema_arity()
    integers_and_manifest()
    manifest_query_whitelist()
    help_catalog()
    print("cli behavior and schemas: PASS")
