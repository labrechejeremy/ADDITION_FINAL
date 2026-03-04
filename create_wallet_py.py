#!/usr/bin/env python3
import subprocess
import sys
import time

def run_daemon_with_commands():
    """Lance le daemon et envoie les commandes"""
    proc = subprocess.Popen(
        [r"c:\Users\admin\Desktop\ADDITION_FINAL\build\additiond.exe"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=r"c:\Users\admin\Desktop\ADDITION_FINAL",
        text=True,
        bufsize=1
    )
    
    try:
        time.sleep(2)  # Attendre que le daemon démarre
        
        # Envoyer la commande createwallet
        stdout, stderr = proc.communicate(input="createwallet\nquit\n", timeout=10)
        
        print("===== OUTPUT DAEMON =====")
        if stdout:
            print(stdout)
        if stderr:
            print("STDERR:", stderr)
            
        return True
    
    except subprocess.TimeoutExpired:
        proc.kill()
        stdout, stderr = proc.communicate()
        print("Timeout:", stdout, stderr)
        return False

if __name__ == "__main__":
    run_daemon_with_commands()
