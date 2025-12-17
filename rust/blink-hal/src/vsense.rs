// src/vsense.rs
#![allow(dead_code)]
#![allow(non_snake_case)]

use rtt_target::rprintln;
use embedded_hal::blocking::i2c::{Write, Read};

/// Resistor dividers (same as the C code)
const MC_R_TOP_OHMS:  i64 = 2_040_000;
const MC_R_BOT_OHMS:  i64 =   12_100;
const BAT_R_TOP_OHMS: i64 = 2_040_000;
const BAT_R_BOT_OHMS: i64 =   12_100;

/// Error type for our simple driver
#[derive(Debug)]
pub enum VsenseError<I2cErr> {
    I2cWrite(I2cErr),
    I2cRead(I2cErr),
    NotReady,      // ADC never became ready (if we add a timeout)
}

/// Build MCP3422 config byte for one-shot conversion on ch=1 or 2,
/// 16-bit resolution, PGA=1 (exactly your C config).
fn mcp3422_cfg_oneshot_ch(ch: u8) -> u8 {
    let ch_bits = if ch == 2 { 1u8 << 5 } else { 0u8 }; // 0x20 for CH2, 0x00 for CH1
    0x80 | ch_bits | 0x10 | 0x08 | 0x00
    //  RDY=1   CH      one-shot  16-bit  PGA=1
}

/// Read one MCP3422 channel and return pin voltage in mV
fn mcp3422_read16_mV_ch<I2C, E>(
    i2c: &mut I2C,
    addr7: u8,
    ch: u8,
) -> Result<i32, VsenseError<E>>
where
    I2C: Write<Error = E> + Read<Error = E>,
    E: core::fmt::Debug, // Added Debug trait to allow printing the error
{
    let cfg = mcp3422_cfg_oneshot_ch(ch);
    
    // CRITICAL FIX: Do NOT shift the address for Rust HAL.
    // C HAL uses 8-bit address (addr << 1), Rust uses 7-bit (addr).
    let addr = addr7; 

    // Start conversion
    if let Err(e) = i2c.write(addr, &[cfg]) {
        rprintln!("VSENSE: Write failed on CH{} (Addr: {:#x}). Error: {:?}", ch, addr, e);
        return Err(VsenseError::I2cWrite(e));
    }

    // Poll until RDY=0.
    let mut rx = [0u8; 3];
    let mut retries = 0; 
    // Add a sanity limit to the loop to prevent hard hangs
    const MAX_RETRIES: u32 = 20_000; 

    loop {
        if let Err(e) = i2c.read(addr, &mut rx) {
            rprintln!("VSENSE: Read failed on poll. Error: {:?}", e);
            return Err(VsenseError::I2cRead(e));
        }

        let status = rx[2];
        if (status & 0x80) == 0 {
            // RDY=0 -> data valid
            break;
        }

        retries += 1;
        if retries > MAX_RETRIES {
            rprintln!("VSENSE: Timeout waiting for RDY on CH{}", ch);
            return Err(VsenseError::NotReady);
        }
    }

    // 16-bit signed code
    let code = i16::from_be_bytes([rx[0], rx[1]]) as i32;

    // LSB = 62.5 µV => mV = code * 0.0625
    // Do it in integer math: uV = code * 625 / 10, then /1000 to get mV
    let uV: i64 = (code as i64 * 625) / 10;
    let mV: i32 = (uV / 1000) as i32;

    Ok(mV)
}

/// Convert ADC pin mV to actual bus mV with the per-channel divider.
fn adc_to_bus_mV_ch(adc_mV: i32, ch: u8) -> i32 {
    if adc_mV < 0 {
        return -1;
    }
    let adc = adc_mV as i64;
    if ch == 2 {
        // Battery bus
        ((adc * (BAT_R_TOP_OHMS + BAT_R_BOT_OHMS)) / BAT_R_BOT_OHMS) as i32
    } else {
        // Motor-controller bus
        ((adc * (MC_R_TOP_OHMS + MC_R_BOT_OHMS)) / MC_R_BOT_OHMS) as i32
    }
}

/// High-level helper:
///  - Trigger one-shot conversions on CH1 and CH2 of MCP3422
///  - Read both channels
///  - Convert to bus mV using the same math as the C code
///  - Return (mc_bus_mV, bat_bus_mV)
pub fn read_mc_and_bat_mV<I2C, E>(
    i2c: &mut I2C,
    addr7: u8,
) -> Result<(i32, i32), VsenseError<E>>
where
    I2C: Write<Error = E> + Read<Error = E>,
    E: core::fmt::Debug, 
{
    // CH1 -> MC bus
    let mc_adc_mV = mcp3422_read16_mV_ch(i2c, addr7, 1)?;
    let mc_bus_mV = adc_to_bus_mV_ch(mc_adc_mV, 1);
    
    // We only print here if successful, otherwise the inner function printed the error
    rprintln!("VSENSE: CH1 Raw: {} mV -> Bus: {} mV", mc_adc_mV, mc_bus_mV);

    // CH2 -> BAT bus
    let bat_adc_mV = mcp3422_read16_mV_ch(i2c, addr7, 2)?;
    let bat_bus_mV = adc_to_bus_mV_ch(bat_adc_mV, 2);

    rprintln!("VSENSE: CH2 Raw: {} mV -> Bus: {} mV", bat_adc_mV, bat_bus_mV);

    Ok((mc_bus_mV, bat_bus_mV))
}