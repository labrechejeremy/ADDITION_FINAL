@echo off
title ADDITION BLOCKCHAIN MAINNET - CMD
rem black background with bright red text for ADDITION logo
color 0C

cd /d "c:\Users\admin\Desktop\ADDITION_FINAL"

echo.
echo ========================================
echo   ADDITION BLOCKCHAIN MAINNET
echo   Commandes: mine, getinfo, createwallet
echo   Quitter: quit ou exit
echo ========================================
echo.

taskkill /F /IM additiond.exe >nul 2>&1

.\build\additiond.exe

pause
