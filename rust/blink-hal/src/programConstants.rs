#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(unused)]

// ----- generated Constants -----

pub const numberOfNodes: u32  = 2;
pub const totalNumFrames: u32 = 2;
pub const numMissingIDs: u32  = 1;
pub const startingOffset: u32 = 8;

// ----- explicitly defined constants -----

pub const nullID: u32               = 0;
pub const pointsPerData: u32        = 10;
pub const nodeIDSizeBits: u32       = 7;
pub const nonCriticalWarning: u32   = 2;  // 0b010
pub const CriticalWarning: u32      = 4;  // 0b100
pub const warningTypeMask: u32      = 7;  // 0b111
pub const warningNodeFlagIndex: u32 = 11;
pub const warningFrameFlagIndex: u32= 18;
pub const maxFrameCntBits: u32      = 3;
pub const warningDataFlagIndex: u32 = 21;
pub const maxDataInFrameBits: u32   = 3;
pub const HBupdateTypeBits: u32     = 1;
pub const HBupdateStatus: u32       = 0;
pub const HBupdateTiming: u32       = 1;
pub const slowestNodeCount: u32     = 3;
pub const HBStatusFrameBits: u32    = 1;
pub const HBTimerMSBits: u32        = 10;
pub const frame0FillerBits: u32     = 11;

// ----- specialIDs -----

pub const vitalsID: u32    = 2;
pub const prechargeID: u32 = 3;
pub const telemetryID: u32 = 4;

// ----- functionCodes -----

pub const CAN_Open_NMT_Function: u32    = 0;
pub const CAN_Open_Synchronization: u32 = 1;
pub const warningCode: u32              = 2;
pub const TelemetryCommand: u32         = 3;
pub const statusUpdate: u32             = 4;
pub const HBPing: u32                   = 5;
pub const HBPong: u32                   = 6;
pub const transmitData: u32             = 7;
pub const HBRespUpdate: u32             = 8;
pub const busStatusUpdate: u32          = 9;
pub const CAN_Open_Err_Cntrl: u32       = 14;

// ----- warningFlags -----

pub const missingFrameFlag: u32   = 16;
pub const frameTimerSetFail: u32  = 32;
pub const dataToHigh: u32         = 64;
pub const dataToLow: u32          = 128;
pub const doubleCritical: u32     = 256;
pub const extrapolate5: u32       = 512;
pub const extrap10: u32           = 1024;

// ----- telemetryCommandFlags -----

pub const enablePrecharge: u32          = 4;
pub const disablePrecharge: u32         = 5;
pub const telemetryCommandAck: u32      = 6;
pub const telemetryCommandCRCError: u32 = 7;
pub const customChangeDataFlag: u32     = 9;

// ----- statusUpdates -----

pub const initFlag: u8                = 0;
pub const canRecoveryFlag: u8         = 1;
pub const canRXOverunFlag: u8         = 2;
pub const prechargeOn_Charging: u8    = 5;
pub const prechargeOn_FinishedCharging: u8 = 6;
pub const prechargeOff: u8            = 7;
