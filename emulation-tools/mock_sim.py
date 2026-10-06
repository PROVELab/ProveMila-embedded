"""Stand-in for Isaac Sim, for testing the QEMU bridge on a machine without it.

It plays the part of the simulation: a throttle profile (what the autonomy stack would output) goes to the firmware as the
pedal, and a tiny vehicle model (a first-order lag with more inertia than the built-in H300 model) turns the motor
command the firmware sends into a speed, which goes back so the emulated motor controller reports it to the MCU.

Usage: python mock_sim.py <bridge udp port> <seconds> "PERCENT@SECONDS,..."
"""
import sys
import time

from qemu_bridge_client import QemuBridge

port, duration = int(sys.argv[1]), float(sys.argv[2])
profile = sorted((float(t), int(p)) for p, t in (item.split("@") for item in sys.argv[3].split(",")))

VEHICLE_TIME_CONSTANT_S = 1.5  # slower than the built-in motor model (0.5 s), so we can tell the sim owns the speed

bridge = QemuBridge(port=port)
speed = 0.0
pedal = profile[0][1]
start = last = time.time()
next_print = 0.0
print(f"{'t(s)':>5} {'throttle %':>10} {'MCU cmd RPM':>12} {'sim speed':>10} {'MCU sees RPM':>13} {'iq (A)':>7}")
while time.time() - start < duration:
    now = time.time()
    dt, last = now - last, now
    t = now - start
    for at, percent in profile:
        if t >= at:
            pedal = percent
    state = bridge.exchange(pedal_pct=pedal, speed_rpm=speed)
    if state:
        command = state.get("reference_rpm", 0.0)
        speed += (command - speed) * min(1.0, dt / VEHICLE_TIME_CONSTANT_S)  # the "vehicle"
    if t >= next_print:
        next_print += 2.0
        if state:
            print(f"{t:5.1f} {pedal:10d} {state.get('reference_rpm', 0):12.1f} {speed:10.1f} "
                  f"{state.get('motor_speed_rpm', 0):13.1f} {state.get('quadrature_current_a', 0):7.2f}", flush=True)
        else:
            print(f"{t:5.1f} (no answer from the bridge yet)", flush=True)
    time.sleep(0.02)  # 50 Hz
bridge.close()
