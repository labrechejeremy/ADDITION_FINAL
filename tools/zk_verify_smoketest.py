#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path


def run(cmd):
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p.returncode, (p.stdout + p.stderr).strip()


def main() -> int:
    wrapper = Path(__file__).resolve().parent / "zk_verify_wrapper.py"
    py = sys.executable

    # invalid proof hex should fail
    rc, out = run([py, str(wrapper), "mint|alice|10|aa|bb", "xyz", "00"])
    if rc == 0:
        print("FAIL: invalid proof hex unexpectedly passed")
        return 1

    # valid hex format but no backend configured -> expected strict failure
    rc, out = run([py, str(wrapper), "mint|alice|10|aa|bb", "00", "00"])
    if rc == 0:
        print("FAIL: verification passed without backend")
        return 2

    print("OK: wrapper strict-mode smoke tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
