// src/hw.rs

#![allow(non_snake_case)]
#![allow(non_camel_case_types)]

use core::cell::RefCell;
use core::cmp;

use cortex_m::interrupt as cm_interrupt;
use cortex_m::interrupt::Mutex;
use cortex_m::peripheral::NVIC;

use bxcan::{self, Fifo, Frame, Id, Interrupt as BxInterrupt, StandardId, ExtendedId};
use nb::Error as NbError;

use stm32f4xx_hal::{
    can::Can as HalCan,
    gpio::{
        gpioa::{PA11, PA12},
        Alternate,
    },
    pac,
    prelude::*,
    rcc::Rcc,
};

use stm32f4xx_hal::pac::interrupt;

use crate::ffi; 
use crate::ffi::{
    CANPacket,
    CANListenParam,
    PCANListenParamsCollection,
    MAX_SIZE_PACKET_DATA,
    // Note: Constants like PCAN_ERR_NOT_RECEIVED are accessed via ffi::
    exact,
    matchID,
    matchFunction,
    sendStatusUpdate,
    pecanInit,
};

// ----------------- CAN globals -----------------

type Can1 = bxcan::Can<HalCan<pac::CAN1>>;

static CAN1_HANDLE: Mutex<RefCell<Option<Can1>>> = Mutex::new(RefCell::new(None));

// Use the bindgen generated name: PCAN_ERR_NOT_RECEIVED
const NOT_RECEIVED: i16 = ffi::PCAN_ERR_NOT_RECEIVED as i16;

// ----------------- Ring buffer -----------------

const PACKET_QUEUE_SIZE: usize = 16;

const EMPTY_PACKET: CANPacket = CANPacket {
    // Cast u32 -> usize
    data: [0; MAX_SIZE_PACKET_DATA as usize], 
    id: 0,
    dataSize: 0,
    rtr: false,      
    extendedID: false, 
};

static mut PACKET_QUEUE: [CANPacket; PACKET_QUEUE_SIZE] = [EMPTY_PACKET; PACKET_QUEUE_SIZE];

static mut QUEUE_HEAD: u8 = 0;
static mut QUEUE_TAIL: u8 = 0;
static mut QUEUE_OVERRUN_FLAG: bool = false;
static mut FLAG_GOT_CALLED: bool = false;

#[inline]
fn increment_index(i: u8) -> u8 {
    let n = i.wrapping_add(1);
    if (n as usize) >= PACKET_QUEUE_SIZE {
        0
    } else {
        n
    }
}

#[inline]
fn queue_is_empty() -> bool {
    unsafe { QUEUE_HEAD == QUEUE_TAIL }
}

#[inline]
unsafe fn queue_packet(pkt: &CANPacket) {
    let next = increment_index(QUEUE_HEAD);
    if next == QUEUE_TAIL {
        QUEUE_OVERRUN_FLAG = true;
    } else {
        PACKET_QUEUE[QUEUE_HEAD as usize] = *pkt;
        QUEUE_HEAD = next;
    }
}

#[inline]
fn queue_packet_pop(out: &mut CANPacket) -> bool {
    let mut ok = false;
    cm_interrupt::free(|_| {
        unsafe {
            if !queue_is_empty() {
                *out = PACKET_QUEUE[QUEUE_TAIL as usize];
                QUEUE_TAIL = increment_index(QUEUE_TAIL);
                ok = true;
            }
        }
    });
    ok
}

// ----------------- matcher[] -----------------

type MatcherFn = unsafe extern "C" fn(u32, u32) -> bool;

static MATCHER: [MatcherFn; 3] = [exact, matchID, matchFunction];

// ----------------- Global node ID -----------------

static mut NODE_ID: i32 = 0;

// ----------------- CAN init (500 kbit/s) -----------------

pub fn can1_init_500k(
    can1: pac::CAN1,
    tx: PA12<Alternate<9>>,
    rx: PA11<Alternate<9>>,
    rcc: &mut Rcc,
) {
    let can_peripheral: HalCan<pac::CAN1> = can1.can((tx, rx), rcc);

    let mut can = bxcan::Can::builder(can_peripheral)
        .set_bit_timing(0x011C_0004)
        .enable();

    let mut filters = can.modify_filters();
    filters.enable_bank(0, Fifo::Fifo0, bxcan::filter::Mask32::accept_all());
    drop(filters);

    can.enable_interrupt(BxInterrupt::Fifo0MessagePending);
    can.enable_interrupt(BxInterrupt::Fifo0Full);
    can.enable_interrupt(BxInterrupt::Fifo0Overrun);

    cm_interrupt::free(|cs| {
        *CAN1_HANDLE.borrow(cs).borrow_mut() = Some(can);
    });

    unsafe {
        NVIC::unmask(pac::Interrupt::CAN1_RX0);
    }
}

// ----------------- ISR helper -----------------

fn on_receive_frame(frame: &Frame) {
    static mut RECV_PACK: CANPacket = EMPTY_PACKET;

    unsafe {
        let pkt = &mut RECV_PACK;

        match frame.id() {
            Id::Standard(sid) => {
                pkt.id = sid.as_raw() as i32;
                pkt.extendedID = false; 
            }
            Id::Extended(eid) => {
                pkt.id = eid.as_raw() as i32;
                pkt.extendedID = true; 
            }
        }

        pkt.rtr = frame.is_remote_frame(); 

        if let Some(data) = frame.data() {
            let bytes: &[u8] = &*data;
            // Cast u32 -> usize
            let len = cmp::min(bytes.len(), MAX_SIZE_PACKET_DATA as usize);
            pkt.dataSize = len as u8;
            pkt.data[..len].copy_from_slice(&bytes[..len]);
        } else {
            pkt.dataSize = 0;
        }

        FLAG_GOT_CALLED = true;
        queue_packet(pkt);
    }
}

// ----------------- CAN1 RX0 interrupt -----------------

#[interrupt]
fn CAN1_RX0() {
    cm_interrupt::free(|cs| {
        let mut handle_ref = CAN1_HANDLE.borrow(cs).borrow_mut();
        if let Some(can) = handle_ref.as_mut() {
            loop {
                match can.receive() {
                    Ok(frame) => on_receive_frame(&frame),
                    Err(_) => break,
                }
            }
        }
    });
}

// ----------------- C-visible: pecan_CanInit -----------------

#[no_mangle]
pub extern "C" fn pecan_CanInit(config: pecanInit) {
    unsafe {
        NODE_ID = config.nodeId;
        sendStatusUpdate(0, NODE_ID as u32);
    }
}

// ----------------- C-visible: waitPackets (non-blocking) -----------------
#[no_mangle]
pub extern "C" fn waitPackets(plpc: *mut PCANListenParamsCollection) -> i16 {
    if plpc.is_null() {
        return NOT_RECEIVED;
    }

    static mut RECV_PACK: CANPacket = EMPTY_PACKET;
    let recv = unsafe { &mut RECV_PACK };

    if !queue_packet_pop(recv) {
        return NOT_RECEIVED;
    }

    if recv.extendedID {
        recv.id &= 0x1FFF_FFFF;
    } else {
        recv.id &= 0x7FF;
    }

    if recv.rtr {
        recv.dataSize = 0;
    } else {
        let sz = recv.dataSize as usize;
        // Cast u32 -> usize
        let clamped = if sz > (MAX_SIZE_PACKET_DATA as usize) {
            MAX_SIZE_PACKET_DATA as usize
        } else {
            sz
        };
        recv.dataSize = clamped as u8;
    }

    let used = recv.dataSize as usize;
    // Cast u32 -> usize
    if used < (MAX_SIZE_PACKET_DATA as usize) {
        for b in &mut recv.data[used..(MAX_SIZE_PACKET_DATA as usize)] {
            *b = 0;
        }
    }

    unsafe {
        let coll = &mut *plpc;

        for i in 0..coll.size {
            let clp = &coll.arr[i as usize]; 

            let mt_idx = clp.mt as i32;
            if mt_idx >= 0 && (mt_idx as usize) < MATCHER.len() {
                let matcher = MATCHER[mt_idx as usize];
                if matcher(recv.id as u32, clp.listen_id) {
                    if let Some(handler) = clp.handler {
                        return handler(recv as *mut CANPacket);
                    }
                }
            }
        }

        if let Some(default_handler) = coll.defaultHandler {
            return default_handler(recv as *mut CANPacket);
        }
    }

    NOT_RECEIVED
}


// ----------------- C-visible: sendPacket -----------------

#[no_mangle]
pub extern "C" fn sendPacket(p: *mut CANPacket) {
    if p.is_null() {
        return;
    }

    let pkt = unsafe { &mut *p };

    // Cast u32 -> usize
    if (pkt.dataSize as usize) > (MAX_SIZE_PACKET_DATA as usize) {
        return;
    }

    if pkt.rtr {
        pkt.dataSize = 1;
    }

    let id_raw = pkt.id as u32;
    let id = if id_raw > 0x7FF {
        ExtendedId::new(id_raw & 0x1FFF_FFFF).map(Id::Extended)
    } else {
        StandardId::new((id_raw & 0x7FF) as u16).map(Id::Standard)
    };

    let id = match id {
        Some(v) => v,
        None => return,
    };

    let frame = if pkt.rtr {
        Frame::new_remote(id, pkt.dataSize)
    } else {
        let len = pkt.dataSize as usize;
        // Cast u32 -> usize
        let len = cmp::min(len, MAX_SIZE_PACKET_DATA as usize);

        match len {
            0 => Frame::new_data(id, [0u8; 0]),
            1 => Frame::new_data(id, [pkt.data[0]]),
            2 => Frame::new_data(id, [pkt.data[0], pkt.data[1]]),
            3 => Frame::new_data(id, [pkt.data[0], pkt.data[1], pkt.data[2]]),
            4 => Frame::new_data(id, [pkt.data[0], pkt.data[1], pkt.data[2], pkt.data[3]]),
            5 => Frame::new_data(
                id,
                [pkt.data[0], pkt.data[1], pkt.data[2], pkt.data[3], pkt.data[4]],
            ),
            6 => Frame::new_data(
                id,
                [
                    pkt.data[0],
                    pkt.data[1],
                    pkt.data[2],
                    pkt.data[3],
                    pkt.data[4],
                    pkt.data[5],
                ],
            ),
            7 => Frame::new_data(
                id,
                [
                    pkt.data[0],
                    pkt.data[1],
                    pkt.data[2],
                    pkt.data[3],
                    pkt.data[4],
                    pkt.data[5],
                    pkt.data[6],
                ],
            ),
            _ => Frame::new_data(
                id,
                [
                    pkt.data[0],
                    pkt.data[1],
                    pkt.data[2],
                    pkt.data[3],
                    pkt.data[4],
                    pkt.data[5],
                    pkt.data[6],
                    pkt.data[7],
                ],
            ),
        }
    };

    loop {
        let mut success = false;

        cm_interrupt::free(|cs| {
            let mut handle_ref = CAN1_HANDLE.borrow(cs).borrow_mut();
            if let Some(can) = handle_ref.as_mut() {
                match can.transmit(&frame) {
                    Ok(_) => success = true,
                    Err(NbError::WouldBlock) => {}
                    Err(_) => {}
                }
            }
        });

        if success {
            break;
        }

        for _ in 0..50_000 {
            cortex_m::asm::nop();
        }
    }
}