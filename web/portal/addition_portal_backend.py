import json
import os
import socket
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

RPC_HOST = os.getenv("ADDITION_RPC_HOST", "127.0.0.1")
RPC_PORT = int(os.getenv("ADDITION_RPC_PORT", "8545"))
HTTP_HOST = os.getenv("ADDITION_HTTP_HOST", "0.0.0.0")
HTTP_PORT = int(os.getenv("ADDITION_HTTP_PORT", "8080"))

def rpc_call(command: str, timeout: float = 4.0) -> str:
    try:
        with socket.create_connection((RPC_HOST, RPC_PORT), timeout=timeout) as s:
            s.sendall((command.strip() + "\n").encode("utf-8"))
            data = s.recv(65535)
            return data.decode("utf-8", errors="ignore").strip()
    except Exception as e:
        return f"error: {str(e)}"

def parse_kv_line(line: str) -> dict:
    out = {}
    if not line or line.startswith("error:"):
        return out
    for item in line.split():
        if "=" in item:
            kv = item.split("=", 1)
            out[kv[0]] = kv[1]
    return out

class PortalHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        path = urlparse(self.path).path
        
        if path == "/api/health":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok", "rpc_host": RPC_HOST}).encode())
            
        elif path == "/api/getinfo":
            info_raw = rpc_call("getinfo")
            info = parse_kv_line(info_raw)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps(info).encode())
        else:
            self.send_response(404)
            self.end_headers()

if __name__ == "__main__":
    print(f"Starting Portal Backend on {HTTP_HOST}:{HTTP_PORT}...")
    server = HTTPServer((HTTP_HOST, HTTP_PORT), PortalHandler)
    server.serve_forever()
