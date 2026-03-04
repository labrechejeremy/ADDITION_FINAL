@echo off
setlocal enabledelayedexpansion

cd /d "c:\Users\admin\Desktop\ADDITION_FINAL"

cls
echo.
echo ========================================
echo   VÉRIFICATION SOLDE BLOCKCHAIN
echo ========================================
echo.

taskkill /F /IM additiond.exe >nul 2>&1
timeout /t 1 /nobreak >nul

echo Adresse: 2748817dadca12c1a5e411b0bc513b5e4a4f0218
echo.
echo Récupération des données...
echo.

(
  echo getbalance 2748817dadca12c1a5e411b0bc513b5e4a4f0218
  echo getinfo
  echo monetary_info
  echo quit
) | .\build\additiond.exe

echo.
echo ========================================
pause
