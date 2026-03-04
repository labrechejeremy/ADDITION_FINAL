@echo off
setlocal enabledelayedexpansion

REM ========================================
REM  ADDITION BLOCKCHAIN MAINNET LAUNCHER
REM ========================================

cd /d "c:\Users\admin\Desktop\ADDITION_FINAL"

echo.
echo ========== CLEAN UP EXISTING PROCESSES ==========
taskkill /F /IM additiond.exe >nul 2>&1
timeout /t 1 /nobreak

echo.
echo ========== STARTING DAEMON & OPERATIONS ==========
echo.

REM Lancer le daemon et envoyer les commandes
(
  echo createwallet
  echo getinfo
  echo mine 345faa9323222d03115f2a0e29b970d70bc495e5
  echo getbalance 345faa9323222d03115f2a0e29b970d70bc495e5
  echo quit
) | .\build\additiond.exe

echo.
echo ========== OPERATIONS COMPLETE ==========
echo.
