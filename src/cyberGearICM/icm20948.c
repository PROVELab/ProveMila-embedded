#include "driver/i2c.h"

#include "icm20948.h"
#include "icm20948_i2c.h"

#define ACK_CHECK_EN   0x1     /* I2C master will check ack from slave */
#define ACK_CHECK_DIS  0x0     /* I2C master will not check ack from slave */


icm20948_status_e icm20948_internal_write_i2c(uint8_t reg, uint8_t *data, uint32_t len, void *user)
{
	icm20948_status_e status = ICM_20948_STAT_OK;
	icm0948_config_i2c_t *args = (icm0948_config_i2c_t*)user;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (args->i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, true);
	i2c_master_write(cmd, data, len, true);
	i2c_master_stop(cmd);
	
	if(i2c_master_cmd_begin(args->i2c_port, cmd, 100 / portTICK_PERIOD_MS) != ESP_OK)
	{
		status = ICM_20948_STAT_ERR;
	}
	i2c_cmd_link_delete(cmd);
    return status;
}

icm20948_status_e icm20948_internal_read_i2c(uint8_t reg, uint8_t *buff, uint32_t len, void *user)
{
	icm20948_status_e status = ICM_20948_STAT_OK;
	icm0948_config_i2c_t *args = (icm0948_config_i2c_t*)user;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (args->i2c_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, true);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (args->i2c_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
	i2c_master_read(cmd, buff, len, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);
	
	if(i2c_master_cmd_begin(args->i2c_port, cmd, 100 / portTICK_PERIOD_MS) != ESP_OK)
	{
		status = ICM_20948_STAT_ERR;
	}
	i2c_cmd_link_delete(cmd);
    return status;
}

/* default serif */
icm20948_serif_t default_serif = {
    icm20948_internal_write_i2c,
    icm20948_internal_read_i2c,
    NULL,
};

void icm20948_init_i2c(icm20948_device_t *icm_device, icm0948_config_i2c_t *args)
{
	icm20948_init_struct(icm_device);
	default_serif.user = (void *)args;
    icm20948_link_serif(icm_device, &default_serif);

#ifdef CONFIG_ICM_20948_USE_DMP
  icm_device->_dmp_firmware_available = true; // Initialize _dmp_firmware_available
#else
  icm_device->_dmp_firmware_available = false; // Initialize _dmp_firmware_available
#endif

    icm_device->_firmware_loaded = false; // Initialize _firmware_loaded
    icm_device->_last_bank = 255;         // Initialize _last_bank. Make it invalid. It will be set by the first call of icm20948_set_bank.
    icm_device->_last_mems_bank = 255;    // Initialize _last_mems_bank. Make it invalid. It will be set by the first call of inv_icm20948_write_mems.
    icm_device->_gyroSF = 0;              // Use this to record the GyroSF, calculated by inv_icm20948_set_gyro_sf
    icm_device->_gyroSFpll = 0;
    icm_device->_enabled_Android_0 = 0;      // Keep track of which Android sensors are enabled: 0-31
    icm_device->_enabled_Android_1 = 0;      // Keep track of which Android sensors are enabled: 32-
    icm_device->_enabled_Android_intr_0 = 0; // Keep track of which Android sensor interrupts are enabled: 0-31
    icm_device->_enabled_Android_intr_1 = 0; // Keep track of which Android sensor interrupts are enabled: 32-

}