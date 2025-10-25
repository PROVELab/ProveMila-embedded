#ifndef progConsts
#define progConsts

// generated Constants
#define numberOfNodes  2
#define totalNumFrames 2
#define numMissingIDs  1
#define startingOffset 8

// Explicilty defined in sensors.def constants
#define nullID                0  // 0
#define pointsPerData         10 // 10
#define nodeIDSizeBits        7  // 7
#define nonCriticalWarning    2  // 0b010
#define CriticalWarning       4  // 0b100
#define warningTypeMask       7  // 0b111
#define warningNodeFlagIndex  11 // 11
#define warningFrameFlagIndex 18 // 18
#define maxFrameCntBits       3  // 3
#define warningDataFlagIndex  21 // 21
#define maxDataInFrameBits    3  // 3
#define HBupdateTypeBits      1  // 1
#define HBupdateStatus        0  // 0b0
#define HBupdateTiming        1  // 0b1
#define slowestNodeCount      3  // 3
#define HBStatusFrameBits     1  // 1
#define HBTimerMSBits         10 // 10
#define frame0FillerBits      11 // 11

// global enum specialIDs
typedef enum {
    vitalsID = 2,    /* 2 */
    prechargeID = 3, /* 3 */
    telemetryID = 4  /* 4 */
} specialIDs;

// global enum functionCodes
typedef enum {
    CAN_Open_NMT_Function = 0,    /* 0b0000 */
    CAN_Open_Synchronization = 1, /* 0b0001 */
    warningCode = 2,              /* 0b0010 */
    TelemetryCommand = 3,         /* 0b0011 */
    statusUpdate = 4,             /* 0b0100 */
    HBPing = 5,                   /* 0b0101 */
    HBPong = 6,                   /* 0b0110 */
    transmitData = 7,             /* 0b0111 */
    HBRespUpdate = 8,             /* 0b1000 */
    busStatusUpdate = 9,          /* 0b1001 */
    CAN_Open_Err_Cntrl = 14       /* 0b1110 */
} functionCodes;

// global enum warningFlags
typedef enum {
    missingFrameFlag = 16,  /* 0b1 << 4 */
    frameTimerSetFail = 32, /* 0b1 << 5 */
    dataToHigh = 64,        /* 0b1 << 6 */
    dataToLow = 128,        /* 0b1 << 7 */
    doubleCritical = 256,   /* 0b1 << 8 */
    extrapolate5 = 512,     /* 0b1 << 9 */
    extrap10 = 1024         /* 0b1 << 10 */
} warningFlags;

// global enum telemetryCommandFlags
typedef enum {
    enablePrecharge = 4,          /* 4 */
    disablePrecharge = 5,         /* 5 */
    telemetryCommandAck = 6,      /* 6 */
    telemetryCommandCRCError = 7, /* 7 */
    customChangeDataFlag = 9      /* 9 */
} telemetryCommandFlags;

// global enum statusUpdates
typedef enum {
    initFlag = 0,                     /* 0b00000000 */
    canRecoveryFlag = 1,              /* 0b00000001 */
    canRXOverunFlag = 2,              /* 0b00000010 */
    prechargeOn_Charging = 5,         /* 0b00000101 */
    prechargeOn_FinishedCharging = 6, /* 0b00000110 */
    prechargeOff = 7                  /* 0b00000111 */
} statusUpdates;

#endif
