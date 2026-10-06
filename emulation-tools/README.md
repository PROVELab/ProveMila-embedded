# emulation-tools

Host-side tools for running the firmware in QEMU (ESP32) on a PC, without hardware.

QEMU has no CAN controller, so in QEMU builds (`-D CMAKE_QEMU_BUILD=ON`) the firmware carries CAN frames over UART1
(see `src/pecan/espSpecific.c`). `can_relay.py` connects to each emulated chip, acts as the CAN bus, and hosts emulated
devices.

## Files

| File | What it is |
|---|---|
| `can_relay.py` | The CAN bus. Joins the UART1 of each QEMU instance; options `--motor`, `--pedal`, `--sim-port`, `--quiet` |
| `h300_emulator.py` | Emulated H300 motor controller (used by `can_relay.py --motor`) |
| `console_probe.py` | Reads the firmware's own status register through its console (`vsr_top`) |
| `qemu_bridge_client.py`, `mock_sim.py`, `isaac_sim_example.py` | Simulator side of the Isaac Sim bridge (UDP + JSON). `isaac_sim_example.py` is untested |
| `demo.ps1` | Builds what is needed and runs a demo: `hello`, `mcu`, `cantest`, `twonode`, `motor`, `pedal`, `sim` |
| `can_test.c` | Test showing the CAN driver does nothing in QEMU |
| `uart_test.c`, `uart_test_CMakeLists.txt`, `relay.py` | Test that two QEMU instances can exchange bytes over UART1 |

## Run a demo

Needs ESP-IDF 5.5.x (installed with the EIM installer, which also installs QEMU) and Python 3. From the repo root,
in PowerShell:

```powershell
.\emulation-tools\demo.ps1 mcu          # run with no name to list all demos
```

The first run of a demo builds the firmware (several minutes). The result is cached in `emulation-tools\demo_tmp`
(git-ignored). After changing `sdkconfig.mcu` or any firmware code, add `-Rebuild`.

## Notes

- The QEMU emulation is for functional tests. QEMU does not run in real time.
- In QEMU, with `CONFIG_FREERTOS_HZ=100` (the value committed in `sdkconfig.mcu`) the firmware asserts in
  `xTaskDelayUntil` right after `Resetting motor` and reboots in a loop: the motor task waits 5 ms between commands,
  which is 0 ticks at 100 Hz. With 1000 it runs stably. It is not known whether the car's own build is affected.
