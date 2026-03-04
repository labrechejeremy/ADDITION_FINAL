#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    # Usage:
    #   python tools/zk_verify_wrapper.py "<public_input>" "<proof_hex>" "<vk_hex>"
    if len(sys.argv) != 4:
        print("ERROR: usage zk_verify_wrapper.py <public_input> <proof_hex> <vk_hex>")
        return 2

    public_input = sys.argv[1].strip()
    proof_hex = sys.argv[2].strip()
    vk_hex = sys.argv[3].strip()

    # Strict format checks
    if not public_input:
        print("ERROR: empty public input")
        return 3

    for name, value in (("proof", proof_hex), ("vk", vk_hex)):
        if not value or len(value) % 2 != 0:
            print(f"ERROR: {name} must be even-length hex")
            return 4
        try:
            bytes.fromhex(value)
        except Exception:
            print(f"ERROR: invalid {name} hex")
            return 5

    backend = os.getenv("ADDITION_ZK_VERIFY_CMD", "").strip()
    if not backend:
        print("ERROR: ADDITION_ZK_VERIFY_CMD not set")
        return 10

    payload = {
        "public_input": public_input,
        "proof_hex": proof_hex,
        "vk_hex": vk_hex,
    }

    with tempfile.TemporaryDirectory(prefix="addition_zk_") as td:
        req = Path(td) / "request.json"
        out = Path(td) / "result.json"
        req.write_text(json.dumps(payload), encoding="utf-8")

        # Contract with backend:
        # backend command receives: <request_json_path> <result_json_path>
        cmd = [backend, str(req), str(out)]

        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
        except subprocess.TimeoutExpired:
            print("ERROR: verifier timeout")
            return 11
        except Exception as e:
            print(f"ERROR: cannot execute verifier: {e}")
            return 12

        if proc.returncode != 0:
            detail = (proc.stdout + "\n" + proc.stderr).strip()
            print(f"ERROR: verifier failed ({proc.returncode}) {detail}")
            return 13

        if not out.exists():
            print("ERROR: verifier did not produce result file")
            return 14

        try:
            result = json.loads(out.read_text(encoding="utf-8"))
        except Exception as e:
            print(f"ERROR: invalid verifier result json: {e}")
            return 15

        if result.get("ok") is True:
            print("OK")
            return 0

        print(f"ERROR: verifier rejected ({result.get('error', 'unknown')})")
        return 16


if __name__ == "__main__":
    raise SystemExit(main())
