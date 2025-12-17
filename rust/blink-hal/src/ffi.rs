// src/ffi.rs
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(unused)]

pub const MAX_SIZE_PACKET_DATA: usize = 8;
pub const MAX_PCAN_PARAMS: usize = 6;

#[repr(i32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum PCAN_ERR {
    PACKET_TOO_BIG = -3,
    NOT_RECEIVED   = -2,
    NOSPACE        = -1,
    SUCCESS        = 0,
    GEN_FAILURE    = 1,
}

#[repr(i32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MATCH_TYPE {
    MATCH_EXACT    = 0,
    MATCH_ID       = 1,
    MATCH_FUNCTION = 2,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct CANPacket {
    pub data: [u8; MAX_SIZE_PACKET_DATA],
    pub id: i32,
    pub dataSize: u8,
    pub rtr: u8,
    pub extendedID: u8,
}

#[repr(C)]
pub struct CANListenParam {
    pub listen_id: u32,
    pub handler: Option<extern "C" fn(*mut CANPacket) -> i16>,
    pub mt: MATCH_TYPE,
}

#[repr(C)]
pub struct PCANListenParamsCollection {
    pub arr: [CANListenParam; MAX_PCAN_PARAMS],
    pub defaultHandler: Option<extern "C" fn(*mut CANPacket) -> i16>,
    pub size: i16,
}

#[repr(C)]
pub struct pecanInit {
    pub nodeId: i32,
    pub pin1: i32,
    pub pin2: i32,
}


extern "C" {
    pub fn vitalsInit(plpc: *mut PCANListenParamsCollection, nodeID: u16);
    pub fn addParam(plpc: *mut PCANListenParamsCollection, clp: CANListenParam) -> i16;

    pub fn combinedID(fn_id: u32, node_id: u32) -> u32;
    pub fn combinedIDExtended(fn_id: u32, node_id: u32, extension: u32) -> u32;

    pub fn exact(id: u32, mask: u32) -> bool;
    pub fn matchID(id: u32, mask: u32) -> bool;
    pub fn matchFunction(id: u32, mask: u32) -> bool;

    pub fn writeData(p: *mut CANPacket, dataPoint: *mut i8, size: i16) -> i16;

    pub fn squeeze(value: i32, min: i32, max: i32) -> i32;

    pub fn setRTR(p: *mut CANPacket) -> i16;
    pub fn setExtended(p: *mut CANPacket) -> i16;

    pub fn formatValue(value: i32, min: i32, max: i32) -> u32;

    pub fn copyValueToData(value: *const u32, target: *mut u8,
                           startBit: i8, numBits: i8) -> i16;
    pub fn copyDataToValue(target: *mut u32, data: *const u8,
                           startBit: i8, numBits: i8) -> i16;

    pub fn defaultPacketRecv(p: *mut CANPacket) -> i16;

    pub fn sendStatusUpdate(flag: u8, Id: u32);

    //previously inline
    pub fn getNodeId(id: u32) -> u32;
    pub fn getFunctionId(id: u32) -> u32;
    pub fn getIdExtension(id: u32) -> u32;
    pub fn getDataFrameId(id: u32) -> u32;
}
