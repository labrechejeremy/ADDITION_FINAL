# Addition (ADD) - Instant Redeploy Script
# Use this to restore the repository to GitHub in one click

# Enter your token when prompted (it will not be saved)
$token = Read-Host "Enter your GitHub Token (e.g. ghp_...)"
if (-not $token) { Write-Error "Token required."; exit }

$REPO_URL = "https://$token@github.com/ADDAddition/ADDITION.git"

Write-Host "Checking Git status..." -ForegroundColor Cyan

if (-not (Test-Path .git)) {
    Write-Host "Initializing new Git repository..." -ForegroundColor Yellow
    git init
}

# Ensure remote is correct
git remote remove origin 2>$null
git remote add origin $REPO_URL

# Sync
Write-Host "Staging files..." -ForegroundColor Cyan
git add .

Write-Host "Committing changes..." -ForegroundColor Cyan
git commit -m "Automated redeploy/backup"

Write-Host "Pushing to GitHub (Force)..." -ForegroundColor Magenta
git branch -M main
git push -u origin main --force

Write-Host "Redeploy Complete!" -ForegroundColor Green
