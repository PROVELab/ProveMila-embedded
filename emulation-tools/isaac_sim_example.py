"""How to connect Isaac Sim to the QEMU relay. NOT TESTED against a real Isaac Sim: the machine this was written on
(RTX 4050 Laptop, 6 GB VRAM) is below Isaac Sim's requirements. Only the bridge side (qemu_bridge_client.py) was tested,
with mock_sim.py standing in for the simulator.

The Isaac Sim specific calls below (SimulationApp, World, add_physics_callback) follow the Isaac Sim 4.5+ standalone
script style; adjust imports and the three functions marked ADAPT to your Isaac Sim version and your vehicle asset.

Run order:
  1. start the QEMU MCU node and:  python can_relay.py 5001 3600 --motor --quiet --sim-port 6000
  2. run this script with Isaac Sim's Python:  ./python.sh isaac_sim_example.py
"""
from qemu_bridge_client import QemuBridge, mps_to_rpm, rpm_to_mps

bridge = QemuBridge(port=6000)  # use the relay machine's address here if QEMU runs on another computer


def get_vehicle_speed_mps():
    """ADAPT: forward speed of the vehicle from the simulation, in m/s (for example from the articulation's root linear
    velocity projected on the heading)."""
    raise NotImplementedError


def get_autonomy_throttle_pct():
    """ADAPT: the throttle the autonomy stack wants, 0..100. It becomes the pedal of the MCU firmware."""
    raise NotImplementedError


def apply_drive_command(motor_rpm_command, quadrature_current_a):
    """ADAPT: drive the wheels. motor_rpm_command is what the MCU firmware commands the motor to do, as RPM
    (rpm_to_mps() converts it to a speed target in m/s)."""
    raise NotImplementedError


def on_physics_step(step_size):
    # 1) send what the simulation knows: throttle (-> MCU pedal) and the real vehicle speed (-> what the motor reports)
    state = bridge.exchange(pedal_pct=get_autonomy_throttle_pct(), speed_rpm=mps_to_rpm(get_vehicle_speed_mps()))
    # 2) apply what the firmware decided
    if state:
        apply_drive_command(state["reference_rpm"], state["quadrature_current_a"])


if __name__ == "__main__":
    from isaacsim import SimulationApp  # must be created before any other Isaac Sim import

    app = SimulationApp({"headless": False})
    from isaacsim.core.api import World

    world = World()
    # ADAPT: add the ground plane and the vehicle here, for example world.scene.add_default_ground_plane()
    world.add_physics_callback("qemu_bridge", on_physics_step)
    world.reset()
    while app.is_running():
        world.step(render=True)
    app.close()
