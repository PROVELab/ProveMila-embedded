# MCU - The heart
The MCU is at the nexus of all Low voltage activity
in Mila, and as such needs to be a little more complex
than just reading sensor data and sending it over CAN.

## Submodule
To run the motor controller code, you need to also clone the submodule.
The easiest way to do this (if you have already cloned this repo) is to run
```
git submodule sync --recursive && git submodule update --recursive
```

If you make changes to the motor code in the subrepo, make sure to commit them,
ideally in a separate branch (but this may not always be possible).
You can use vscode's git view to make this easier.

## Notes before running
- Verify motor controller can timeout ms and baud

## VSR architecture
- The MCU manages state via the VSR (Vehicle State Register)
- This is basically a big struct that different tasks update.
    It's code is found in vsr.h and mutexes are initiallized in vsr.c
- Each substruct of the vsr is mutexed, so different
    cores can access different parts of the vsr at different
    times
- The vsr must be marked volatile, otherwise writes to it 
    may be optimized to registers by the compiler

## Tasks

- The esp32 has 2 cores running at 240 Mhz each
- We split the CAN buses: one specific for motor controller
    (high priority) and one for sensors (lower priority)

### Core 0
- Core 0 has the following, critical tasks:
    - Lowest priority tasks (i.e. always running):
        Read Data from CAN bus and put it into relevant
        queue (only motor controller queue for now)
    - High priority task: At 200 hz, send current RPM
    - High priority task: Block on queue, and when
        there are things in the queue, parse them
        and update the VSR

### Core 1
- TODO