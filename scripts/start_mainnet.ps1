$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Assert-Ok([string]$step) {
    if ($LASTEXITCODE -ne 0) {
        throw "[mainnet] $step failed with exit code $LASTEXITCODE"
    }
}

# Backup and sanitize potentially corrupted state files
$dataDir = Join-Path $root "data"
if (Test-Path $dataDir) {
    $ts = Get-Date -Format "yyyyMMdd_HHmmss"
    $backupDir = Join-Path $root "data_backup_$ts"
    New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
    $dataItems = Get-ChildItem -Path $dataDir -Force -ErrorAction SilentlyContinue
    if ($dataItems) {
        Copy-Item -Path (Join-Path $dataDir "*") -Destination $backupDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    foreach ($f in @("blocks.dat", "mempool.dat", "staking.dat", "contracts.dat", "tokens.dat", "bridge.dat", "privacy.dat")) {
        $p = Join-Path $dataDir $f
        if (Test-Path $p) {
            try {
                Get-Content -Path $p -Raw | Out-Null
            } catch {
                Remove-Item -Path $p -Force -ErrorAction SilentlyContinue
            }
        }
    }
}

Get-Process additiond -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

Write-Host "[mainnet] building..."
cmake -S . -B build
Assert-Ok "cmake configure"
cmake --build build
Assert-Ok "cmake build"

$exe = Join-Path $root "build\additiond.exe"
if (-not (Test-Path $exe)) {
    throw "additiond.exe not found"
}

Write-Host "[mainnet] starting daemon..."
& $exe
