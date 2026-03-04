# Addition (ADD) Mainnet Runbook

## 1) Pre-flight checklist
- Build must succeed:
  - `cmake -S . -B build`
  - `cmake --build build`
- `additiond.exe` exists in `build/`
- liboqs/OpenSSL available on host
- Firewall rules configured for:
  - TCP 18545 (LAN RPC, optional public)
  - TCP 28545 (P2P transport)
- Data backup path ready (`./data` snapshot)

## 2) Security baseline
- Keep `node_identity.dat` private and backed up.
- Configure privacy verifier command before ZK operations:
  - `privacy_set_verifier python tools/zk_verify_wrapper.py`
- Set environment var for real verifier backend:
  - `ADDITION_ZK_VERIFY_CMD=<your_real_verifier_cmd>`

## 3) First startup
- Launch daemon:
  - `build\additiond.exe`
- Verify startup self-test line contains:
  - `selftest: ok`
- Verify listeners:
  - `local RPC listening on 127.0.0.1:8545`
  - `P2P RPC listening on 0.0.0.0:28545`

## 4) Health checks
- `getinfo`
- `monetary_info`
- `crypto_selftest`
- `peers`

Expected:
- `pq_mode=strict`
- `max_supply=50000000`
- `crypto_selftest` returns `ok:selftest: ok`

## 5) Network bootstrap
- Add known bootstrap peers:
  - `addpeer <ip:port>`
- Trigger sync:
  - `sync`
- Confirm chain progress:
  - `getinfo`

## 6) Wallet and transaction sanity
- Create wallet:
  - `createwallet`
- Mine one block to bootstrap address:
  - `mine <address>`
- Send a tx:
  - `sendtx_hash <from> <pub> <priv> <to> <amount> <fee> <nonce>`
- Track status:
  - `tx_status <tx_hash>`
- Instant receive check:
  - `getbalance_instant <to_address>`

## 7) Privacy strict sanity
- Configure verifier:
  - `privacy_set_verifier <wrapper_cmd>`
- Submit ZK mint/spend only (avoid non-ZK in strict operations):
  - `privacy_mint_zk ...`
  - `privacy_spend_zk ...`

## 8) Portal + metrics
- Start portal backend:
  - `python web/portal/addition_portal_backend.py`
- Open portal:
  - `web/portal/index.html`
- Check `/api/health`, `/api/dashboard`

## 9) Mainnet go/no-go
Go only if all true:
- crypto self-test pass at boot and via command
- peers connected and sync stable
- tx submit + mined path working
- monetary cap telemetry sane
- no repeated P2P ban storms

## 10) Rollback protocol
- Stop daemon cleanly (`quit`)
- Snapshot `./data` to timestamped backup
- Restore last known-good `./data` if anomaly detected
- Relaunch and verify:
  - `getinfo`
  - `monetary_info`
  - `crypto_selftest`

## 11) Operational cadence
- Every hour:
  - `getinfo`, `peers`, `monetary_info`
- Every day:
  - backup `./data`
  - verify portal health endpoint
- Every release:
  - rebuild, self-test, staged restart, post-checks
