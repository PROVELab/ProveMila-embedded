"""Client for the QEMU relay's simulator bridge (can_relay.py --motor --sim-port N).

Pure Python standard library, no Isaac imports, so it can be used inside Isaac Sim's Python (for example from a physics
step callback or a standalone script), in a ROS node, or in a test.

What goes over UDP (JSON):
  simulator -> bridge   {"pedal_pct": 0..100, "speed_rpm": <vehicle/motor speed from the simulation>}
                        either key is optional. pedal_pct becomes the emulated accel pedal of the MCU firmware.
                        speed_rpm becomes the speed the emulated H300 motor controller reports to the MCU.
  bridge -> simulator   {"reference_rpm": <what the MCU commands the motor to do>, "motor_speed_rpm", "quadrature_current_a",
                         "dc_voltage_v", "pedal_pct", "pwm_ref_sel", "reference_raw", "discharge_limit_pct",
                         "charge_limit_pct", "mcu_frames", "t"}
"""
import json
import socket

# Wheel circumference from src/mcu/motor_h300/h300.h (MILA_WHEEL_CIRCUMFERENCE = 86.64 in). The firmware's MPH_TO_RPM
# treats motor RPM as wheel RPM (1:1), so these helpers do the same.
WHEEL_CIRCUMFERENCE_M = 86.64 * 0.0254


def mps_to_rpm(speed_mps):
    """Vehicle speed in m/s -> motor RPM, as the firmware computes it."""
    return speed_mps / WHEEL_CIRCUMFERENCE_M * 60.0


def rpm_to_mps(rpm):
    """Motor RPM -> vehicle speed in m/s."""
    return rpm / 60.0 * WHEEL_CIRCUMFERENCE_M


class QemuBridge:
    def __init__(self, host="127.0.0.1", port=6000):
        self._addr = (host, port)
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.setblocking(False)
        self.state = {}  # last state received from the bridge

    def exchange(self, pedal_pct=None, speed_rpm=None):
        """Send our values (if any), then read every state packet that has arrived. Never blocks.
        Returns the newest state dict (empty until the bridge has answered once)."""
        msg = {}
        if pedal_pct is not None:
            msg["pedal_pct"] = pedal_pct
        if speed_rpm is not None:
            msg["speed_rpm"] = speed_rpm
        # always send something, even an empty object: the bridge learns our address from the first packet
        self._sock.sendto(json.dumps(msg).encode("utf-8"), self._addr)
        while True:
            try:
                data, _ = self._sock.recvfrom(4096)
            except (BlockingIOError, InterruptedError):
                break
            except OSError:
                break
            try:
                self.state = json.loads(data.decode("utf-8"))
            except ValueError:
                pass
        return self.state

    def close(self):
        self._sock.close()
