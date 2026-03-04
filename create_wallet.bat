@echo off
setlocal enabledelayedexpansion

cd /d "c:\Users\admin\Desktop\ADDITION_FINAL"

echo [Creating Wallet]
(
echo createwallet
echo quit
) | .\build\additiond.exe

pause
