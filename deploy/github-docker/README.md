# GitHub + Docker Launch Folder

This folder is the **clear deploy entrypoint** for running Addition online with Docker.

## Files
- `Dockerfile` : builds `additiond` daemon image
- `docker-compose.yml` : runs node + portal backend
- `run-local.ps1` : one-command launch on Windows

## Quick Start
From repo root:

```powershell
powershell -ExecutionPolicy Bypass -File deploy/github-docker/run-local.ps1
```

Then open:
- Portal API: `http://127.0.0.1:8080/api/health`
- Node RPC (TCP): `127.0.0.1:8545`

## Production API domain (xa1.ai)

Recommended:
- Frontend: `https://xa1.ai`
- Backend API: `https://api.xa1.ai`

The compose file already wires backend -> daemon with:
- `ADDITION_RPC_HOST=additiond`
- `ADDITION_RPC_PORT=8545`

Test API after deploy:
- `https://api.xa1.ai/api/health`
- `https://api.xa1.ai/api/getinfo`

Then open frontend with API override once:
- `https://xa1.ai/?api=https://api.xa1.ai`

The portal stores this API URL in browser localStorage.

## Publish on GitHub
1. Push repository to GitHub
2. Configure GitHub Actions (workflow in `.github/workflows/docker-image.yml`)
3. Build/push image to GHCR
4. Deploy with your server using that image
