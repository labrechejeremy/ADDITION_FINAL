@echo off
setlocal enabledelayedexpansion

cd /d "c:\Users\admin\Desktop\ADDITION_FINAL"

REM Terminer les processus existants
taskkill /F /IM additiond.exe >nul 2>&1
timeout /t 1 /nobreak >nul

REM Lancer le daemon avec le mining
(
  echo mine 2748817dadca12c1a5e411b0bc513b5e4a4f0218
  echo getbalance 2748817dadca12c1a5e411b0bc513b5e4a4f0218
  echo getinfo
  echo quit
) | .\build\additiond.exe

exit /b 0
