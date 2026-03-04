$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location "$root\deploy\cloudflare"

if (-not (Get-Command npx -ErrorAction SilentlyContinue)) {
    throw "npx not found. Install Node.js first."
}

Write-Host "[cloudflare] Deploying static portal with Wrangler"
npx wrangler deploy

Write-Host "[cloudflare] Done"
