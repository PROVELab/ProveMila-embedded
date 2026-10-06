"""Relay raw bytes between the UART1 TCP sockets of several QEMU instances (simple shared bus)."""
import socket
import sys
import threading
import time

ports = [int(p) for p in sys.argv[1].split(",")]
duration = float(sys.argv[2])
socks = []


def connect(port):
    for _ in range(100):
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=2)
            s.settimeout(None)
            return s
        except OSError:
            time.sleep(0.2)
    raise SystemExit(f"could not connect to port {port}")


def pump(i):
    while True:
        try:
            data = socks[i].recv(256)
        except OSError:
            return
        if not data:
            return
        print(f"relay: port {ports[i]} -> others: {data!r}", flush=True)
        for j, s in enumerate(socks):
            if j != i:
                try:
                    s.sendall(data)
                except OSError:
                    pass


socks.extend(connect(p) for p in ports)
print("relay: connected to all", flush=True)
for i in range(len(socks)):
    threading.Thread(target=pump, args=(i,), daemon=True).start()
time.sleep(duration)
