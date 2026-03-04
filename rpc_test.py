import socket
import sys

HOST='127.0.0.1'
PORT=8545
try:
    s = socket.create_connection((HOST, PORT), timeout=5)
    s.settimeout(5)
    s.sendall(b'getinfo\n')
    data = s.recv(1024)
    print('response', data)
    s.close()
except Exception as e:
    print('error', e)
    sys.exit(1)
