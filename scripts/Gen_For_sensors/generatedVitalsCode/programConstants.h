#ifndef progConsts
#define progConsts

//generated Constants
#define numberOfNodes 4
#define totalNumFrames 6
#define numMissingIDs 1
#define startingOffset 6

//Explicilty defined in sensors.def constants
#define pointsPerData 10		// 10
#define vitalsID     2		// 2
#define prechargeID  3		// 3
#define telemetryID  4		// 4
#define nonCriticalWarning 2		// 0b010
#define CriticalWarning  4		// 0b100
#define missedFrame  16		// 0b1 << 4
#define frameTimerSetFail  32		// 0b1 << 5
#define dataOutOfBounds  64		// 0b1 << 6
#define dataToHigh  128		// 0b1 << 7
#define dataToLow  0		// 0b0 << 7
#define doubleCritical  256		// 0b01 << 8
#define extrapolate5  512		// 0b10 << 8
#define extrap10  768		// 0b11 << 8
#define CriticalTypeMask 768		// 0b11 << 8
#define warningFrameFlagIndex 10		// 10
#define maxFrameCntBits 3		// 3
#define warningDataFlagIndex 13		// 13
#define maxDataInFrameBits 3		// 3
#define numHBRespFrames  2		// 2
#define HBTimerMSBits  10		// 10
#define frame0FillerBits  10		// 10

// global enum functionCodes
typedef enum {
	CAN_Open_NMT_Function = 0,	/* 0b0000 */
	CAN_Open_Synchronization = 1,	/* 0b0001 */
	warningCode = 2,	/* 0b0010 */
	TelemetryCommand = 3,	/* 0b0011 */
	statusUpdate = 4,	/* 0b0100 */
	HBPing = 5,	/* 0b0101 */
	HBPong = 6,	/* 0b0110 */
	transmitData = 7,	/* 0b0111 */
	HBRespUpdate = 8,	/* 0b1000 */
	busStatusUpdate = 9,	/* 0b1001 */
	CAN_Open_Err_Cntrl = 14	/* 0b1110 */
} functionCodes;

// global enum telemetryCommandFlags
typedef enum {
	enablePrecharge = 4,	/* 4 */
	disablePrecharge = 5,	/* 5 */
	telemetryCommandAck = 6,	/* 6 */
	telemetryCommandCRCError = 7,	/* 7 */
	customChangeDataFlag = 9	/* 9 */
} telemetryCommandFlags;

// global enum statusUpdates
typedef enum {
	initFlag = 0,	/* 0b00000000 */
	canRecoveryFlag = 1,	/* 0b00000001 */
	prechargeStatus = 4,	/* 0b00000100 */
	prechargeOn_Charging = 5,	/* 0b00000101 */
	prechargeOn_FinishedCharging = 6,	/* 0b00000110 */
	prechargeOff = 7	/* 0b00000111 */
} statusUpdates;

#endif
