"""CAN bus relay for QEMU ESP32 nodes whose CAN is carried over UART1 (see src/pecan/espSpecific.c, QEMU_BUILD).

Connects to the UART1 TCP socket of each QEMU instance, decodes the 16-byte frames, prints them, and forwards every
valid frame to all the other nodes (a shared bus). Optionally injects a frame periodically, to act as a simple
emulated device.

Frame (16 bytes): [0]=0xA5 sync, [1]=flags (bit0 extended, bit1 rtr), [2..5]=id little endian, [6]=dlc,
[7..14]=data, [15]=XOR of bytes 1..14.

Usage:
  python can_relay.py 5001,5002 60
  python can_relay.py 5001 60 --inject-id 0x080 --inject-data "01 02 03" --inject-period 1.0
"""
import argparse
import json
import socket
import threading
import time

SYNC = 0xA5
FRAME_LEN = 16

parser = argparse.ArgumentParser()
parser.add_argument("ports", help="comma separated TCP ports (QEMU UART1 servers)")
parser.add_argument("seconds", type=float, help="how long to run")
parser.add_argument("--inject-id", type=lambda x: int(x, 0))
parser.add_argument("--inject-data", default="")
parser.add_argument("--inject-period", type=float, default=1.0)
parser.add_argument("--motor", action="store_true", help="emulate the H300 motor controller (h300_emulator.py)")
parser.add_argument("--quiet", action="store_true", help="only print a summary, not every frame")
parser.add_argument("--pedal", help='emulated accel pedal schedule "PERCENT@SECONDS,..." counted from the first frame '
                                    'the MCU sends, e.g. "50@0,100@10,0@20"')
parser.add_argument("--sim-port", type=int, help="UDP port for the simulator bridge (Isaac Sim); needs --motor")
parser.add_argument("--sim-rate", type=float, default=50.0, help="how many state packets per second go to the simulator")
args = parser.parse_args()

# CAN id of the QEMU pedal control frame: combinedID(0b1101, 8) in src/sensors/pedalSensor/pedal_sensor.c
PEDAL_SET_ID = (0b1101 << 7) + 8
first_frame_time = []  # filled with the time of the first frame from a QEMU node

motor = None
if args.motor:
    from h300_emulator import H300Emulator

    motor = H300Emulator()

ports = [int(p) for p in args.ports.split(",")]
socks = []
lock = threading.Lock()
counts = {p: 0 for p in ports}
seen = {}        # can id -> frames seen from QEMU nodes
seen_motor = {}  # can id -> frames sent by the emulated motor controller


def checksum(frame):
    c = 0
    for b in frame[1:FRAME_LEN - 1]:
        c ^= b
    return c


def encode(can_id, data, extended=None, rtr=False):
    if extended is None:
        extended = can_id > 0x7FF
    frame = bytearray(FRAME_LEN)
    frame[0] = SYNC
    frame[1] = (1 if extended else 0) | (2 if rtr else 0)
    frame[2:6] = can_id.to_bytes(4, "little")
    frame[6] = 0 if rtr else len(data)
    frame[7:7 + len(data)] = data
    frame[15] = checksum(frame)
    return bytes(frame)


def describe(frame):
    ext, rtr = frame[1] & 1, (frame[1] >> 1) & 1
    can_id = int.from_bytes(frame[2:6], "little")
    data = frame[7:7 + frame[6]].hex(" ")
    return f"id=0x{can_id:X} {'ext' if ext else 'std'}{' rtr' if rtr else ''} dlc={frame[6]} data=[{data}]"


def connect(port):
    for _ in range(150):
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=2)
            s.settimeout(None)
            return s
        except OSError:
            time.sleep(0.2)
    raise SystemExit(f"could not connect to port {port}")


def pump(i):
    buf = bytearray()
    while True:
        try:
            data = socks[i].recv(256)
        except OSError:
            return
        if not data:
            return
        buf += data
        while buf:
            if buf[0] != SYNC:
                del buf[0]  # not at a frame start, resync
                continue
            if len(buf) < FRAME_LEN:
                break  # wait for the rest of the frame
            frame = bytes(buf[:FRAME_LEN])
            if checksum(frame) != frame[15]:
                del buf[0]  # bad checksum, resync
                continue
            del buf[:FRAME_LEN]
            counts[ports[i]] += 1
            if not first_frame_time:
                first_frame_time.append(time.time())
            seen[int.from_bytes(frame[2:6], "little")] = seen.get(int.from_bytes(frame[2:6], "little"), 0) + 1
            if not args.quiet:
                print(f"[{time.strftime('%H:%M:%S')}] port {ports[i]} -> bus: {describe(frame)}", flush=True)
            with lock:
                for j, s in enumerate(socks):
                    if j != i:
                        try:
                            s.sendall(frame)
                        except OSError:
                            pass
            if int.from_bytes(frame[2:6], "little") == REFERENCE_ID and frame[6] >= 5:
                last_reference.update(pwm_ref_sel=frame[7], reference_raw=int.from_bytes(frame[8:10], "little"),
                                      discharge_limit_pct=frame[10], charge_limit_pct=frame[11])
            if motor is not None:
                can_id = int.from_bytes(frame[2:6], "little")
                for reply_id, reply_data in motor.on_frame(can_id, frame[7:7 + frame[6]]):
                    send_from_motor(reply_id, reply_data)


def send_from_motor(can_id, data):
    frame = encode(can_id, data)
    seen_motor[can_id] = seen_motor.get(can_id, 0) + 1
    if not args.quiet:
        print(f"[{time.strftime('%H:%M:%S')}] motor -> bus: {describe(frame)}", flush=True)
    with lock:
        for s in socks:
            try:
                s.sendall(frame)
            except OSError:
                pass


REFERENCE_ID = 10  # H300 reference message the MCU sends to the motor controller
last_reference = {"pwm_ref_sel": 0, "reference_raw": 0, "discharge_limit_pct": 0, "charge_limit_pct": 0}
sim_state = {"addr": None, "pedal_pct": None, "packets_in": 0, "packets_out": 0}


def send_pedal(percent):
    frame = encode(PEDAL_SET_ID, bytes([max(0, min(100, int(percent)))]))
    with lock:
        for s in socks:
            try:
                s.sendall(frame)
            except OSError:
                pass


def sim_receive_loop(sock):
    """The simulator sends JSON like {"pedal_pct": 40, "speed_rpm": 123.4}. Either key is optional."""
    while True:
        try:
            data, addr = sock.recvfrom(4096)
        except OSError:
            return
        try:
            msg = json.loads(data.decode("utf-8"))
        except ValueError:
            continue  # ignore garbage
        sim_state["addr"] = addr  # answer whoever talked to us last
        sim_state["packets_in"] += 1
        if "speed_rpm" in msg and motor is not None:
            motor.set_external_speed(msg["speed_rpm"])
        if "pedal_pct" in msg:
            percent = max(0, min(100, int(msg["pedal_pct"])))
            if percent != sim_state["pedal_pct"]:  # the firmware holds the value, so only send changes
                sim_state["pedal_pct"] = percent
                send_pedal(percent)


def sim_send_loop(sock):
    """Streams the firmware's side of the loop to the simulator."""
    while True:
        time.sleep(1.0 / args.sim_rate)
        if sim_state["addr"] is None:
            continue
        active = bool(last_reference["pwm_ref_sel"] & 0b100)
        state = {
            "t": time.time(),
            "reference_rpm": last_reference["reference_raw"] * 0.125 if active else 0.0,  # what the MCU commands
            "reference_raw": last_reference["reference_raw"],
            "pwm_ref_sel": last_reference["pwm_ref_sel"],
            "discharge_limit_pct": last_reference["discharge_limit_pct"],
            "charge_limit_pct": last_reference["charge_limit_pct"],
            "motor_speed_rpm": motor.speed_rpm,  # what the emulated H300 reports to the MCU
            "quadrature_current_a": motor.quadrature_arms,
            "dc_voltage_v": 320.0,
            "pedal_pct": sim_state["pedal_pct"],
            "mcu_frames": sum(counts.values()),
        }
        try:
            sock.sendto(json.dumps(state).encode("utf-8"), sim_state["addr"])
            sim_state["packets_out"] += 1
        except OSError:
            pass


def pedal_loop():
    schedule = sorted((float(t), int(p)) for p, t in (item.split("@") for item in args.pedal.split(",")))
    while not first_frame_time:
        time.sleep(0.05)
    start = first_frame_time[0]
    for at, percent in schedule:
        time.sleep(max(0.0, start + at - time.time()))
        frame = encode(PEDAL_SET_ID, bytes([percent]))
        print(f"[{time.strftime('%H:%M:%S')}] PEDAL set to {percent}% (t+{at:g}s)", flush=True)
        with lock:
            for s in socks:
                try:
                    s.sendall(frame)
                except OSError:
                    pass


def motor_tick_loop():
    while True:
        time.sleep(0.002)
        for reply_id, reply_data in motor.tick():
            send_from_motor(reply_id, reply_data)


def inject_loop():
    payload = bytes.fromhex(args.inject_data)
    frame = encode(args.inject_id, payload)
    while True:
        time.sleep(args.inject_period)
        print(f"[{time.strftime('%H:%M:%S')}] relay -> bus: {describe(frame)}", flush=True)
        with lock:
            for s in socks:
                try:
                    s.sendall(frame)
                except OSError:
                    pass


socks.extend(connect(p) for p in ports)
print(f"relay: connected to ports {ports}", flush=True)
for i in range(len(socks)):
    threading.Thread(target=pump, args=(i,), daemon=True).start()
if args.inject_id is not None:
    threading.Thread(target=inject_loop, daemon=True).start()
if motor is not None:
    threading.Thread(target=motor_tick_loop, daemon=True).start()
if args.pedal:
    threading.Thread(target=pedal_loop, daemon=True).start()
if args.sim_port:
    if motor is None:
        raise SystemExit("--sim-port needs --motor")
    sim_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sim_sock.bind(("0.0.0.0", args.sim_port))
    print(f"relay: simulator bridge listening on UDP {args.sim_port}", flush=True)
    threading.Thread(target=sim_receive_loop, args=(sim_sock,), daemon=True).start()
    threading.Thread(target=sim_send_loop, args=(sim_sock,), daemon=True).start()
time.sleep(args.seconds)
if args.sim_port:
    print(f"relay: simulator packets in={sim_state['packets_in']} out={sim_state['packets_out']}", flush=True)
print("relay: frames seen per port:", counts, flush=True)
print("relay: frames from QEMU nodes by id:", {hex(k): v for k, v in sorted(seen.items())}, flush=True)
if motor is not None:
    print("relay: frames from emulated motor by id:", {k: v for k, v in sorted(seen_motor.items())}, flush=True)
    print(f"relay: motor saw {motor.resets} reset(s), {motor.references} reference frame(s)", flush=True)
