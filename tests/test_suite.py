from argparse import ArgumentParser
from pathlib import Path
from dataclasses import dataclass, field
from typing import List

import subprocess as sp
import termcolor
import yaml
from difflib import unified_diff

@dataclass
class TestCase:
    name: str
    input: str
    checks: List[str] = field(default_factory=lambda: ["stdout", "stderr", "exitcode"])

OK_TAG = f"[ {termcolor.colored('OK', 'green')} ]"
KO_TAG = f"[ {termcolor.colored('KO', 'red')} ]"

def diff(expected: str, actual: str) -> str:
    expected_lines = expected.splitlines(keepends=True)
    actual_lines = actual.splitlines(keepends=True)
    return ''.join(unified_diff(expected_lines, actual_lines, fromfile='expected', tofile='actual'))


def run_shell(shell: str, stdin: str) -> sp.CompletedProcess:
    return sp.run([shell], input=stdin, capture_output=True, text=True)

def perform_checks(expected: sp.CompletedProcess, actual: sp.CompletedProcess, checks):
    assert "has_stderr" not in checks or actual.stderr != "", \
        "Something was expected on stderr"

    assert "exitcode" not in checks or expected.returncode == actual.returncode, \
            f"Exited with {actual.returncode} expected {expected.returncode}\n\n{actual.stderr}"

    assert "stdout" not in checks or expected.stdout == actual.stdout, \
            f"stdout differ\n{diff(expected.stdout, actual.stdout)}"

    assert "stderr" not in checks or expected.stderr == actual.stderr, \
            f"stderr differ\n{diff(expected.stderr, actual.stderr)}"


if __name__ == "__main__":
    parser = ArgumentParser("Testsuite")
    parser.add_argument("--binary", required=True, type=Path)
    args = parser.parse_args()

    binary_path = args.binary.absolute()
    print(f"Testing {binary_path}")

    with open("tests.yml", "r") as file:
        testsuite = [TestCase(**testcase) for testcase in yaml.safe_load(file)]
    test_nb = 0
    test_sucess = 0
    for testcase in testsuite:
        stdin = testcase.input
        name = testcase.name
        dash_proc = run_shell("dash", stdin)
        sh_proc = run_shell(binary_path, stdin)
        test_nb += 1
        try:
            perform_checks(dash_proc, sh_proc, testcase.checks)
        except AssertionError as err:
            print(f"{KO_TAG} {name}\n{err}")
        else:
            test_sucess += 1
            print(f"{OK_TAG} {name}")

    print("\nTotal: ")
    print(f"{OK_TAG} {test_sucess}")
    print(f"{KO_TAG} {test_nb - test_sucess}")
    print(str(test_sucess*100/test_nb) + " % of success")
