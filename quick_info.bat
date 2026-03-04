@echo off
cd /d "c:\Users\admin\Desktop\ADDITION_FINAL"
taskkill /F /IM additiond.exe >nul 2>&1
timeout /t 1 /nobreak >nul

(
  echo getinfo
  echo quit
) | .\build\additiond.exe 2>&1
