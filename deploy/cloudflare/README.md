# Cloudflare Launch Folder

Deploy the Addition portal to Cloudflare Workers + Assets.

## Files
- `wrangler.toml`
- `worker.js`
- `deploy_cloudflare.ps1`

## Prerequisites
- Node.js installed
- Cloudflare account + Wrangler auth

## Login
```powershell
npx wrangler login
```

## Deploy
```powershell
powershell -ExecutionPolicy Bypass -File deploy/cloudflare/deploy_cloudflare.ps1
```

## Notes
- This deploys **portal frontend** globally on Cloudflare.
- Your backend API (`/api/*`) should run separately (Docker/VPS) and be referenced by DNS / reverse proxy.

## xa1.ai setup
- Frontend URL: `https://xa1.ai`
- Backend API URL: `https://api.xa1.ai`

Open once with:
- `https://xa1.ai/?api=https://api.xa1.ai`

If you still see `Failed to fetch`, validate API directly:
- `https://api.xa1.ai/api/health`
