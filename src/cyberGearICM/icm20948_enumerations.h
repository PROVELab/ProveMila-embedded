#ifndef _ICM_20948_ENUMERATIONS_H_
#define _ICM_20948_ENUMERATIONS_H_

  // // Generalized
  // REG_BANK_SEL = 0x7F,

  // // Gyroscope and Accelerometer
  // // User Bank 0
  // AGB0_REG_WHO_AM_I = 0x00,
  // 	// Break
  // AGB0_REG_USER_CTRL = 0x03,
  // 	// Break
  // AGB0_REG_LP_CONFIG = 0x05,

  typedef enum
  {
    SAMPLE_MODE_CONTINUOUS = 0x00,
    SAMPLE_MODE_CYCLED,
  } icm20948_lp_config_cycle_e;

  // AGB0_REG_PWR_MGMT_1,

  typedef enum
  {
    CLOCK_INTERNAL_20MHZ = 0x00,
    CLOCK_AUTO,
    CLOCK_TIMING_RESET = 0x07
  } icm20948_pwr_mgmt_1_clksel_e;

  // AGB0_REG_PWR_MGMT_2,
  // 	// Break
  // AGB0_REG_INT_PIN_CONFIG = 0x0F,
  // AGB0_REG_INT_ENABLE,
  // AGB0_REG_INT_ENABLE_1,
  // AGB0_REG_INT_ENABLE_2,
  // AGB0_REG_INT_ENABLE_3,
  // 	// Break
  // AGB0_REG_I2C_MST_STATUS = 0x17,
  // 	// Break
  // AGB0_REG_INT_STATUS = 0x19,
  // AGB0_REG_INT_STATUS_1,
  // AGB0_REG_INT_STATUS_2,
  // AGB0_REG_INT_STATUS_3,
  // 	// Break
  // AGB0_REG_DELAY_TIMEH = 0x28,
  // AGB0_REG_DELAY_TIMEL,
  // 	// Break
  // AGB0_REG_ACCEL_XOUT_H = 0x2D,
  // AGB0_REG_ACCEL_XOUT_L,
  // AGB0_REG_ACCEL_YOUT_H,
  // AGB0_REG_ACCEL_YOUT_L,
  // AGB0_REG_ACCEL_ZOUT_H,
  // AGB0_REG_ACCEL_ZOUT_L,
  // AGB0_REG_GYRO_XOUT_H,
  // AGB0_REG_GYRO_XOUT_L,
  // AGB0_REG_GYRO_YOUT_H,
  // AGB0_REG_GYRO_YOUT_L,
  // AGB0_REG_GYRO_ZOUT_H,
  // AGB0_REG_GYRO_ZOUT_L,
  // AGB0_REG_TEMP_OUT_H,
  // AGB0_REG_TEMP_OUT_L,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_00,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_01,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_02,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_03,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_04,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_05,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_06,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_07,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_08,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_09,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_10,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_11,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_12,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_13,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_14,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_15,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_16,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_17,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_18,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_19,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_20,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_21,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_22,
  // AGB0_REG_EXT_PERIPH_SENS_DATA_23,
  // 	// Break
  // AGB0_REG_FIFO_EN_1 = 0x66,
  // AGB0_REG_FIFO_EN_2,
  // AGB0_REG_FIFO_MODE,
  // 	// Break
  // AGB0_REG_FIFO_COUNT_H = 0x70,
  // AGB0_REG_FIFO_COUNT_L,
  // AGB0_REG_FIFO_R_W,
  // 	// Break
  // AGB0_REG_DATA_RDY_STATUS = 0x74,
  // 	// Break
  // AGB0_REG_FIFO_CFG = 0x76,
  // 	// Break
  // AGB0_REG_MEM_START_ADDR 	= 0x7C,		// Hmm, Invensense thought they were sneaky not listing these locations on the datasheet...
  // AGB0_REG_MEM_R_W 			= 0x7D,		// These three locations seem to be able to access some memory within the device
  // AGB0_REG_MEM_BANK_SEL 		= 0x7E,		// And that location is also where the DMP image gets loaded
  // AGB0_REG_REG_BANK_SEL = 0x7F,

  // // Bank 1
  // AGB1_REG_SELF_TEST_X_GYRO = 0x02,
  // AGB1_REG_SELF_TEST_Y_GYRO,
  // AGB1_REG_SELF_TEST_Z_GYRO,
  // 	// Break
  // AGB1_REG_SELF_TEST_X_ACCEL = 0x0E,
  // AGB1_REG_SELF_TEST_Y_ACCEL,
  // AGB1_REG_SELF_TEST_Z_ACCEL,
  // 	// Break
  // AGB1_REG_XA_OFFS_H = 0x14,
  // AGB1_REG_XA_OFFS_L,
  // 	// Break
  // AGB1_REG_YA_OFFS_H = 0x17,
  // AGB1_REG_YA_OFFS_L,
  // 	// Break
  // AGB1_REG_ZA_OFFS_H = 0x1A,
  // AGB1_REG_ZA_OFFS_L,
  // 	// Break
  // AGB1_REG_TIMEBASE_CORRECTION_PLL = 0x28,
  // 	// Break
  // AGB1_REG_REG_BANK_SEL = 0x7F,

  // // Bank 2
  // AGB2_REG_GYRO_SMPLRT_DIV = 0x00,

  /*
Gyro sample rate divider. Divides the internal sample rate to generate the sample
rate that controls sensor data output rate, FIFO sample rate, and DMP sequence rate.
NOTE: This register is only effective when FCHOICE = 1’b1 (FCHOICE_B register bit is 1’b0), and
(0 < DLPF_CFG < 7).
ODR is computed as follows:
1.1 kHz/(1+GYRO_SMPLRT_DIV[7:0])
*/

  // AGB2_REG_GYRO_CONFIG_1,

  typedef enum
  { // Full scale range options in degrees per second
    DPS_250 = 0x00,
    DPS_500,
    DPS_1000,
    DPS_2000,
  } icm20948_gyro_config_1_fs_sel_e;

  typedef enum
  { // Format is dAbwB_nXbwY - A is integer part of 3db BW, B is fraction. X is integer part of nyquist bandwidth, Y is fraction
    GYR_D196BW6_N229BW8 = 0x00,
    GYR_D151BW8_N187BW6,
    GYR_D119BW5_B154BW3,
    GYR_D51BW2_N73BW3,
    GYR_D23BW9_N35BW9,
    GYR_D11BW6_N17BW8,
    GYR_D5BW7_N8BW9,
    GYR_D361BW4_N376BW5,
  } icm20948_gyro_config_1_dlpcfg_e;

  // AGB2_REG_GYRO_CONFIG_2,
  // AGB2_REG_XG_OFFS_USRH,
  // AGB2_REG_XG_OFFS_USRL,
  // AGB2_REG_YG_OFFS_USRH,
  // AGB2_REG_YG_OFFS_USRL,
  // AGB2_REG_ZG_OFFS_USRH,
  // AGB2_REG_ZG_OFFS_USRL,
  // AGB2_REG_ODR_ALIGN_EN,
  // 	// Break
  // AGB2_REG_ACCEL_SMPLRT_DIV_1 = 0x10,
  // AGB2_REG_ACCEL_SMPLRT_DIV_2,
  // AGB2_REG_ACCEL_INTEL_CTRL,
  // AGB2_REG_ACCEL_WOM_THR,
  // AGB2_REG_ACCEL_CONFIG,

  typedef enum
  {
    GPM_2 = 0x00,
    GPM_4,
    GPM_8,
    GPM_16,
  } icm20948_accel_config_fs_sel_e;

  typedef enum
  { // Format is dAbwB_nXbwZ - A is integer part of 3db BW, B is fraction. X is integer part of nyquist bandwidth, Y is fraction
    ACC_D246BW_N265BW = 0x00,
    ACC_D246BW_N265BW_1,
    ACC_D111BW4_N136BW,
    ACC_D50BW4_N68BW8,
    ACC_D23BW9_N34BW4,
    ACC_D11BW5_N17BW,
    ACC_D5BW7_N8BW3,
    ACC_D473BW_N499BW,
  } icm20948_accel_config_dlpcfg_e;

  // AGB2_REG_ACCEL_CONFIG_2,
  // 	// Break
  // AGB2_REG_FSYNC_CONFIG = 0x52,
  // AGB2_REG_TEMP_CONFIG,
  // AGB2_REG_MOD_CTRL_USR,
  // 	// Break
  // AGB2_REG_REG_BANK_SEL = 0x7F,

  // // Bank 3
  // AGB3_REG_I2C_MST_ODR_CONFIG = 0x00,
  // AGB3_REG_I2C_MST_CTRL,
  // AGB3_REG_I2C_MST_DELAY_CTRL,
  // AGB3_REG_I2C_PERIPH0_ADDR,
  // AGB3_REG_I2C_PERIPH0_REG,
  // AGB3_REG_I2C_PERIPH0_CTRL,
  // AGB3_REG_I2C_PERIPH0_DO,
  // AGB3_REG_I2C_PERIPH1_ADDR,
  // AGB3_REG_I2C_PERIPH1_REG,
  // AGB3_REG_I2C_PERIPH1_CTRL,
  // AGB3_REG_I2C_PERIPH1_DO,
  // AGB3_REG_I2C_PERIPH2_ADDR,
  // AGB3_REG_I2C_PERIPH2_REG,
  // AGB3_REG_I2C_PERIPH2_CTRL,
  // AGB3_REG_I2C_PERIPH2_DO,
  // AGB3_REG_I2C_PERIPH3_ADDR,
  // AGB3_REG_I2C_PERIPH3_REG,
  // AGB3_REG_I2C_PERIPH3_CTRL,
  // AGB3_REG_I2C_PERIPH3_DO,
  // AGB3_REG_I2C_PERIPH4_ADDR,
  // AGB3_REG_I2C_PERIPH4_REG,
  // AGB3_REG_I2C_PERIPH4_CTRL,
  // AGB3_REG_I2C_PERIPH4_DO,
  // AGB3_REG_I2C_PERIPH4_DI,
  // 	// Break
  // AGB3_REG_REG_BANK_SEL = 0x7F,

  // // Magnetometer
  // M_REG_WIA2 = 0x01,
  // 	// Break
  // M_REG_ST1 = 0x10,
  // M_REG_HXL,
  // M_REG_HXH,
  // M_REG_HYL,
  // M_REG_HYH,
  // M_REG_HZL,
  // M_REG_HZH,
  // M_REG_ST2,
  // 	// Break
  // M_REG_CNTL2 = 0x31,
  // M_REG_CNTL3,
  // M_REG_TS1,
  // M_REG_TS2,

#endif /* _ICM_20948_ENUMERATIONS_H_ */