#!/usr/bin/env python3
"""Call a Ghidra MCP bridge tool over stdio.

Usage:
  uv run python scripts/analysis/ghidra_call.py list                 # list tool names
  uv run python scripts/analysis/ghidra_call.py call <tool> '<json>' # call tool
  uv run python scripts/analysis/ghidra_call.py call <tool> k=v k2=v2
"""
import json, os, subprocess, sys, threading, queue

def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    mode = sys.argv[1]
    env = dict(os.environ, GHIDRA_MCP_ALLOW_SCRIPTS="1")
    p = subprocess.Popen(
        ["uv", "run", "--directory", "~/Git/ghidra-mcp", "bridge-mcp-ghidra"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, env=env,
        text=True, bufsize=1)
    q = queue.Queue()
    def reader():
        for line in p.stdout:
            line = line.strip()
            if line.startswith("{"):
                try: q.put(json.loads(line))
                except Exception: pass
    threading.Thread(target=reader, daemon=True).start()

    def rpc(method, params=None, _id=None):
        msg = {"jsonrpc": "2.0", "method": method}
        if params is not None: msg["params"] = params
        if _id is not None: msg["id"] = _id
        p.stdin.write(json.dumps(msg) + "\n"); p.stdin.flush()
        if _id is None: return None
        while True:
            m = q.get(timeout=300)
            if m.get("id") == _id:
                if "error" in m: raise RuntimeError(m["error"])
                return m.get("result")

    if mode == "list":
        rpc("initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                           "clientInfo": {"name": "h", "version": "0"}}, 0)
        rpc("notifications/initialized")
        r = rpc("tools/list", {}, 1)
        names = [t["name"] for t in r.get("tools", [])]
        open("scratch/ghidra_tools.txt", "w").write("\n".join(names))
        print("\n".join(names))
    elif mode == "call":
        tool, args = sys.argv[2], {}
        for a in sys.argv[3:]:
            if "=" in a:
                k, v = a.split("=", 1)
                try: args[k] = json.loads(v)
                except Exception: args[k] = v
            else:
                args[a] = True
        rpc("initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                           "clientInfo": {"name": "h", "version": "0"}}, 0)
        rpc("notifications/initialized")
        r = rpc("tools/call", {"name": tool, "arguments": args}, 1)
        content = r.get("content", r)
        for c in content:
            if c.get("type") == "text":
                print(c["text"])
    p.kill()

if __name__ == "__main__":
    main()
