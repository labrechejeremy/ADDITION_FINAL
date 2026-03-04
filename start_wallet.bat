@echo off
REM launch the wallet UI; ensure the daemon is already running
cd /d "c:\Users\admin\Desktop\ADDITION_FINAL"
if not exist web\addition_wallet_pro.py (
    echo wallet script not found
    pause
    exit /b 1
)
echo Starting Addition wallet...
python web\addition_wallet_pro.py
