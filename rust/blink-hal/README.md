# Blink with HAL

Blinks the NUCLEO-F103RB board LED connected to pin 5 on Port A using the `stm32f1xx-hal` crate.

## Building
```bash
cargo build --release```

## Flashing
```bash
cargo flash --chip stm32f405RGTx --release
```
## Flashing with debug prints:
cargo embed --release

"upload and monitor:"
cargo build --release && cargo embed --release

Also, install the target, may be necessary:
rustup target add thumbv7em-none-eabihf
