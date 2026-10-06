"""Emulated EMDrive H300 motor controller for the CAN relay.

Speaks the protocol the MCU firmware expects (src/mcu/motor_h300/h300.h, h300.c, motor_send.c), SLAVE_ID = 1:

  MCU -> motor  id 10   reference   [0]=pwm_ref_sel (bit2 = reference active), [1,2]=reference (u16 LE, 0.125 RPM),
                                    [3]=discharge limit %, [4]=charge limit %
  MCU -> motor  id 103  reset       [0]=1
  motor -> MCU  id 100  high freq   i16 LE: [0,1] quadrature current (0.125 A), [2,3] direct current, [4,5] speed RPM
                                    (sent in answer to every reference message)
  motor -> MCU  id 101  medium      [0,1] DC voltage (0.125 V), [2,3] DC current (0.125 A, i16), [4,5] current limit (u16)
                                    every 10 ms
  motor -> MCU  id 102  low         [0] protection, [1] safety error, [2] motor temp, [3] inverter temp, [4] bus cap
                                    temp (bytes = deg C + 60), [5] pwm status; every 100 ms
  motor -> MCU  id 150  protections 1 (1 Hz),  id 151  protections 2 (1 Hz)

The physics is a simple model, not the real inverter: the speed follows the reference with a first-order lag.
"""
import struct
import time

REFERENCE_ID = 10
RESET_ID = 103
HIGH_ID = 100
MED_ID = 101
LOW_ID = 102
PROT1_ID = 150
PROT2_ID = 151

SPEED_TIME_CONSTANT_S = 0.5
DC_VOLTAGE_V = 320.0
CURRENT_LIMIT_ARMS = 400.0
MIN_HIGH_PERIOD_S = 0.005  # cap the high frequency replies at 200 Hz of wall-clock time


def _i16(value):
    return max(-32768, min(32767, int(value)))


class H300Emulator:
    def __init__(self):
        self.speed_rpm = 0.0
        self.target_rpm = 0.0
        self.active = False
        self.quadrature_arms = 0.0
        self.external_speed_rpm = None  # when set, the speed comes from a simulator instead of the built-in lag model
        self.resets = 0
        self.references = 0
        self._last_update = None
        self._last_high = float("-inf")
        self._next_med = 0.0
        self._next_low = 0.0
        self._next_prot = 0.0

    # --- model -------------------------------------------------------------------------------------------------
    def _advance(self, now):
        if self._last_update is None or now <= self._last_update:
            self._last_update = now if self._last_update is None else self._last_update
            return
        dt = now - self._last_update
        self._last_update = now
        prev = self.speed_rpm
        if self.external_speed_rpm is not None:
            self.speed_rpm = self.external_speed_rpm  # a simulator owns the speed (see set_external_speed)
        else:
            self.speed_rpm += (self.target_rpm - self.speed_rpm) * min(1.0, dt / SPEED_TIME_CONSTANT_S)
        accel = (self.speed_rpm - prev) / dt
        # torque current: a little to hold speed plus whatever is needed to accelerate
        self.quadrature_arms = (0.02 * self.speed_rpm + 0.05 * accel) if self.active else 0.0

    # --- frames ------------------------------------------------------------------------------------------------
    def _high(self):
        return HIGH_ID, struct.pack("<hhh", _i16(self.quadrature_arms * 8), 0, _i16(self.speed_rpm))

    def _medium(self):
        dc_current = abs(self.quadrature_arms) * 0.5 + 1.0
        return MED_ID, struct.pack("<hhH", _i16(DC_VOLTAGE_V * 8), _i16(dc_current * 8), int(CURRENT_LIMIT_ARMS * 8))

    def _low(self):
        motor_c = 40 + self.speed_rpm / 100.0
        return LOW_ID, bytes([0, 0, int(motor_c) + 60, 35 + 60, 30 + 60, 1 if self.active else 0])

    @staticmethod
    def _prot1():
        # can timeout 1000 ms, regen 50 (-2A units), traction 100 (3A), stall type 1, stall time 50 (4 ms), stall current
        # 50 (2A), overspeed 200 (10 RPM)
        return PROT1_ID, struct.pack("<HBBBBBB", 1000, 50, 100, 1, 50, 50, 200)

    @staticmethod
    def _prot2():
        # max motor temp 110 C, gain 10, max inverter temp 70 C, gain 5, id overcurrent 150 (2A), overvoltage 200 (2V),
        # shutdown 100 (2V)
        return PROT2_ID, bytes([110, 10, 70, 5, 150, 200, 100])

    def set_external_speed(self, rpm):
        """Let a simulator (for example Isaac Sim) own the motor speed. The motor then reports this speed."""
        self.external_speed_rpm = float(rpm)

    # --- interface used by the relay ---------------------------------------------------------------------------
    def on_frame(self, can_id, data, now=None):
        """Handle a frame seen on the bus. Returns a list of (id, data) frames the motor sends in reply."""
        now = time.monotonic() if now is None else now
        replies = []
        if can_id == RESET_ID:
            self.resets += 1
            self.speed_rpm = self.target_rpm = 0.0
            self.active = False
        elif can_id == REFERENCE_ID and len(data) >= 5:
            self.references += 1
            pwm_ref_sel, reference = data[0], struct.unpack("<H", data[1:3])[0]
            self.active = bool(pwm_ref_sel & 0b100)
            self.target_rpm = reference * 0.125 if self.active else 0.0
            self._advance(now)
            if now - self._last_high >= MIN_HIGH_PERIOD_S:
                self._last_high = now
                replies.append(self._high())
        return replies

    def tick(self, now=None):
        """Periodic frames. Call often (every few ms). Returns a list of (id, data)."""
        now = time.monotonic() if now is None else now
        self._advance(now)
        out = []
        if now >= self._next_med:
            self._next_med = now + 0.010
            out.append(self._medium())
        if now >= self._next_low:
            self._next_low = now + 0.100
            out.append(self._low())
        if now >= self._next_prot:
            self._next_prot = now + 1.0
            out.append(self._prot1())
            out.append(self._prot2())
        return out


if __name__ == "__main__":
    # Self-test of the model: a reference of 4000 (= 500 RPM) should ramp the speed up, neutral should bring it back.
    emu = H300Emulator()
    t = 0.0
    emu.on_frame(RESET_ID, bytes([1]), now=t)
    for _ in range(300):
        t += 0.01
        emu.on_frame(REFERENCE_ID, struct.pack("<BHBB", 0b100, 4000, 50, 50), now=t)
    print(f"after 3 s at reference 4000: speed={emu.speed_rpm:.0f} RPM (target 500)")
    assert 480 < emu.speed_rpm <= 500
    for _ in range(300):
        t += 0.01
        emu.on_frame(REFERENCE_ID, struct.pack("<BHBB", 0, 0, 100, 100), now=t)
    print(f"after 3 s in neutral:        speed={emu.speed_rpm:.0f} RPM (target 0)")
    assert emu.speed_rpm < 5
    frames = emu.tick(now=t + 1)
    print("tick frames:", [(i, d.hex(" ")) for i, d in frames])
    print("self-test OK")
