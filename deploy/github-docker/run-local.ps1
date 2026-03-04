$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $root

Write-Host "[deploy] Starting Addition stack with Docker Compose"
docker compose -f deploy/github-docker/docker-compose.yml up -d --build

Write-Host "[deploy] Stack started:"
Write-Host "- Node RPC: http://127.0.0.1:8545"
Write-Host "- Portal API: http://127.0.0.1:8080"
