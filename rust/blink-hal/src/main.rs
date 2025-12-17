#![no_std]
#![no_main]
#![allow(non_snake_case)]

use panic_halt as _;
use cortex_m_rt::entry;

// REPLACE iprintln with RTT
use rtt_target::{rtt_init_print, rprintln};

use stm32f4xx_hal::{
    pac,
    prelude::*,
    i2c::{I2c, Mode},
    rcc::Config,
};

mod ffi {
    #![allow(non_upper_case_globals)]
    #![allow(non_camel_case_types)]
    #![allow(non_snake_case)]
    #![allow(dead_code)]

    // Includes bindings from c_src/wrapper.h
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
mod hw;
mod vsense;
// mod programConstants; // Removed (now in ffi)

use crate::ffi::{
    CANPacket,
    CANListenParam,
    PCANListenParamsCollection,
    MATCH_TYPE,
    pecanInit,
    MAX_SIZE_PACKET_DATA,
    combinedID,
    addParam,
    sendStatusUpdate,
    
    // Bindgen Generated Constants
    functionCodes_HBPing,
    functionCodes_HBPong,
    specialIDs_vitalsID,
    specialIDs_prechargeID,
    MATCH_TYPE_MATCH_EXACT, 
};

// use crate::programConstants::*; // Removed

use crate::hw::{can1_init_500k, pecan_CanInit, sendPacket, waitPackets};

/// Callback invoked by waitPackets when a matching packet is received.
#[no_mangle]
pub extern "C" fn respond_to_hb(_p: *mut CANPacket) -> i16 {
    // Print inside callback using RTT
    rprintln!("responding to HBPing");

    let mut pkt = CANPacket {
        // Cast u32 constant to usize for array size
        data: [0; MAX_SIZE_PACKET_DATA as usize], 
        id: 0,
        dataSize: 0,
        rtr: true,       
        extendedID: false, 
    };

    unsafe {
        // Use the long bindgen names here
        pkt.id = combinedID(functionCodes_HBPong, specialIDs_prechargeID) as i32;
    }
    
    sendPacket(&mut pkt as *mut CANPacket);

    0 // success
}

#[entry]
fn main() -> ! {
    // 1. Initialize RTT Printing
    rtt_init_print!();
    rprintln!("*** ENTERED MAIN (RTT) ***");

    let dp = pac::Peripherals::take().unwrap();
    let mut cp = cortex_m::Peripherals::take().unwrap();

    // 2. Clocks: 80 MHz SYSCLK from HSI
    let mut rcc = dp.RCC.freeze(Config::hsi().sysclk(80.MHz()));

    // 3. GPIO Setup
    let gpioa = dp.GPIOA.split(&mut rcc);
    let gpiob = dp.GPIOB.split(&mut rcc);
    
    let mut led = gpioa.pa3.into_push_pull_output();

    // I2C1 Pins: PB6=SCL, PB7=SDA
    let scl = gpiob.pb6
        .into_alternate::<4>()
        .set_open_drain()
        .internal_pull_up(true); 

    let sda = gpiob.pb7
        .into_alternate::<4>()
        .set_open_drain()
        .internal_pull_up(true); 

    // CAN1 Pins: PA12=TX, PA11=RX
    let tx = gpioa.pa12.into_alternate::<9>();
    let rx = gpioa.pa11.into_alternate::<9>();

    // 4. Peripherals Initialization
    
    let mut i2c1 = I2c::new(
        dp.I2C1,
        (scl, sda),
        Mode::standard(100.kHz()),
        &mut rcc,
    );

    rprintln!("--- I2C SCAN START ---");
    for addr in 1u8..128 {
        match i2c1.write(addr, &[]) {
            Ok(_) => {
                rprintln!("Found device at address: {:#x} (decimal: {})", addr, addr);
            },
            Err(_) => {} 
        }
    }
    rprintln!("--- I2C SCAN END ---");

    can1_init_500k(dp.CAN1, tx, rx, &mut rcc);

    let cfg = pecanInit {
        nodeId: 1, 
        pin1: -1,
        pin2: -1,
    };
    pecan_CanInit(cfg);

    let mut listen_collection: PCANListenParamsCollection =
        unsafe { core::mem::zeroed() };

    unsafe {
        // Use the long bindgen names here
        let listen_id = combinedID(functionCodes_HBPing, specialIDs_vitalsID);
        
        let baby_duck = CANListenParam {
            listen_id,
            handler: Some(respond_to_hb),
            mt: MATCH_TYPE_MATCH_EXACT, 
        };
        addParam(&mut listen_collection as *mut _, baby_duck);
    }
    
    let mut delay = cp.SYST.delay(&rcc.clocks);
    let mcp3422_addr7: u8 = 0x69;

    // 5. Main Loop
    loop {
        rprintln!("looping");
        
        led.set_high();
        
        unsafe { sendStatusUpdate(5, 3); }

        let _res = waitPackets(&mut listen_collection as *mut _);

        match vsense::read_mc_and_bat_mV(&mut i2c1, mcp3422_addr7) {
            Ok((mc_mV, bat_mV)) => {
                 let mc_v = mc_mV / 1000;
                 let bat_v = bat_mV / 1000;
                 rprintln!("MC: {}V, BAT: {}V", mc_v, bat_v);
            }
            Err(_) => {
                 rprintln!("VSENSE Read Error");
            }
        }
        
        led.set_low();
        delay.delay_ms(1000_u32);
    }
}