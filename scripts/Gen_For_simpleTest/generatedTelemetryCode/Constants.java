public final class Constants {
    private Constants() {}

    //generated Constants
    public static final int numberOfNodes = 2;
    public static final int totalNumFrames = 2;
    public static final int numMissingIDs = 1;
    public static final int startingOffset = 8;

    //Explicilty defined in sensors.def constants
    public static final int nullID  = 0;		// 0
    public static final int pointsPerData = 10;		// 10
    public static final int nodeIDSizeBits  = 7;		// 7
    public static final int nonCriticalWarning = 2;		// 0b010
    public static final int CriticalWarning  = 4;		// 0b100
    public static final int warningTypeMask  = 7;		// 0b111
    public static final int warningNodeFlagIndex  = 11;		// 11
    public static final int warningFrameFlagIndex = 18;		// 18
    public static final int maxFrameCntBits = 3;		// 3
    public static final int warningDataFlagIndex = 21;		// 21
    public static final int maxDataInFrameBits = 3;		// 3
    public static final int HBupdateTypeBits  = 1;		// 1
    public static final int HBupdateStatus  = 0;		// 0b0
    public static final int HBupdateTiming  = 1;		// 0b1
    public static final int slowestNodeCount  = 3;		// 3
    public static final int HBStatusFrameBits  = 1;		// 1
    public static final int HBTimerMSBits  = 10;		// 10
    public static final int frame0FillerBits  = 11;		// 11

	// global enum specialIDs
	public static final class specialIDs {
		private specialIDs() {}
		public static final int vitalsID = 2;	// 2
		public static final int prechargeID = 3;	// 3
		public static final int telemetryID = 4;	// 4
	}

	// global enum functionCodes
	public static final class functionCodes {
		private functionCodes() {}
		public static final int CAN_Open_NMT_Function = 0;	// 0b0000
		public static final int CAN_Open_Synchronization = 1;	// 0b0001
		public static final int warningCode = 2;	// 0b0010
		public static final int TelemetryCommand = 3;	// 0b0011
		public static final int statusUpdate = 4;	// 0b0100
		public static final int HBPing = 5;	// 0b0101
		public static final int HBPong = 6;	// 0b0110
		public static final int transmitData = 7;	// 0b0111
		public static final int HBRespUpdate = 8;	// 0b1000
		public static final int busStatusUpdate = 9;	// 0b1001
		public static final int CAN_Open_Err_Cntrl = 14;	// 0b1110
	}

	// global enum warningFlags
	public static final class warningFlags {
		private warningFlags() {}
		public static final int missingFrameFlag = 16;	// 0b1 << 4
		public static final int frameTimerSetFail = 32;	// 0b1 << 5
		public static final int dataToHigh = 64;	// 0b1 << 6
		public static final int dataToLow = 128;	// 0b1 << 7
		public static final int doubleCritical = 256;	// 0b1 << 8
		public static final int extrapolate5 = 512;	// 0b1 << 9
		public static final int extrap10 = 1024;	// 0b1 << 10
	}

	// global enum telemetryCommandFlags
	public static final class telemetryCommandFlags {
		private telemetryCommandFlags() {}
		public static final int enablePrecharge = 4;	// 4
		public static final int disablePrecharge = 5;	// 5
		public static final int telemetryCommandAck = 6;	// 6
		public static final int telemetryCommandCRCError = 7;	// 7
		public static final int customChangeDataFlag = 9;	// 9
	}

	// global enum statusUpdates
	public static final class statusUpdates {
		private statusUpdates() {}
		public static final int initFlag = 0;	// 0b00000000
		public static final int canRecoveryFlag = 1;	// 0b00000001
		public static final int canRXOverunFlag = 2;	// 0b00000010
		public static final int prechargeOn_Charging = 5;	// 0b00000101
		public static final int prechargeOn_FinishedCharging = 6;	// 0b00000110
		public static final int prechargeOff = 7;	// 0b00000111
	}

	// global enum twaiState
	public static final class twaiState {
		private twaiState() {}
		public static final int TWAI_STATE_STOPPED = 0;	// 0
		public static final int TWAI_STATE_RUNNING = 1;	// 1
		public static final int TWAI_STATE_BUS_OFF = 2;	// 2
		public static final int TWAI_STATE_RECOVERING = 3;	// 3
	}

	public static final int[] nodeIDs = new int[]{ 8, 10 };
}
