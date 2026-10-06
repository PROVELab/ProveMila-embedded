"""Talk to the MCU firmware console (UART0 exposed by QEMU as a TCP server) and print VSR topics.

Usage: python console_probe.py <uart0 port> <boot wait seconds> <topic> [<topic> ...]
Runs `vsr_top <topic> 2` for each topic, waits a moment, presses Enter to stop it, and prints the last block printed.
"""
import re
import socket
import sys
import time

port, boot_wait, topics = int(sys.argv[1]), float(sys.argv[2]), sys.argv[3:]


def connect():
    for _ in range(150):
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=2)
            s.settimeout(0.2)
            return s
        except OSError:
            time.sleep(0.2)
    raise SystemExit(f"could not connect to console port {port}")


def read_for(sock, seconds):
    out = b""
    end = time.time() + seconds
    while time.time() < end:
        try:
            chunk = sock.recv(4096)
            if not chunk:
                break
            out += chunk
        except socket.timeout:
            pass
        except OSError:
            break
    return out.decode("utf-8", "replace")


def field(block, name):
    m = re.search(rf"{re.escape(name)}:\s*(-?[\d.]+)", block)
    return m.group(1) if m else "?"


sock = connect()
read_for(sock, boot_wait)  # let the firmware boot and reach the console prompt
if topics and topics[0] == "series":  # python console_probe.py <port> <boot wait> series <seconds>
    start = time.time()
    sock.sendall(b"vsr_top all 4\n")
    text = read_for(sock, float(topics[1]))
    sock.sendall(b"\n")
    print(f"{'fw time (s)':>12} {'pedal %':>8} {'reference':>10} {'speed RPM':>10}")
    last = None
    for block in re.split(r"\x1b\[2J\x1b\[H", text):
        if "pedal_position_pct" not in block or "motor_speed:" not in block:
            continue
        stamp = re.search(r"motor_speed:\s*timestamp:\s*(\d+) us", block)
        seconds = f"{int(stamp.group(1)) / 1e6:.1f}" if stamp else "?"
        row = (field(block, "pedal_position_pct"), field(block, "speed_reference (in 8ths)"), field(block, "\n  motor_speed"))
        if row != last:  # only print when something changed
            print(f"{seconds:>12} {row[0]:>8} {row[1]:>10} {row[2]:>10}")
            last = row
    sock.close()
    raise SystemExit(0)
for topic in topics:
    sock.sendall(f"vsr_top {topic} 2\n".encode())
    text = read_for(sock, 3.0)
    sock.sendall(b"\n")
    text += read_for(sock, 0.7)
    blocks = [b for b in re.split(r"\x1b\[2J\x1b\[H", text) if f"{topic}:" in b]
    print(f"===== {topic} =====")
    if blocks:
        # the firmware's pedal debug prints share this UART, so cut each block where they start
        block = blocks[-1].split("collecting")[0]
        print("\n".join(line for line in block.splitlines() if line.strip()))
    else:
        print("(no output; raw tail: " + repr(text[-200:]) + ")")
sock.close()
