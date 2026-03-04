# Addition ZK Verifier Backend Contract

The wrapper `tools/zk_verify_wrapper.py` now expects a **real external verifier backend**.

## Environment Variable
Set:
- `ADDITION_ZK_VERIFY_CMD`

Value example:
- `python C:/path/to/your_real_verifier.py`
- `C:/path/to/native_verifier.exe`

## Invocation Contract
Wrapper calls backend as:

`<ADDITION_ZK_VERIFY_CMD> <request_json_path> <result_json_path>`

## Input File (`request_json_path`)
JSON format:
```json
{
  "public_input": "string",
  "proof_hex": "hex string",
  "vk_hex": "hex string"
}
```

## Output File (`result_json_path`)
JSON format:
```json
{
  "ok": true,
  "error": "optional error message"
}
```

- If `ok: true` => wrapper returns `OK` and exit code `0`.
- Any other output is treated as rejection.

## Strictness
- Wrapper validates that `proof_hex` and `vk_hex` are even-length valid hex before backend call.
- Wrapper fails if backend exits non-zero, times out, or returns invalid output.
- No internal fake verification remains.
