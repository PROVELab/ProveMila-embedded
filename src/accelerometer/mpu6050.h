#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>

#define MPU_ADDRESS 0x68
#define MPU_PWR_MGMT_1 0x6B
#define MPU_SIG_PATH_RST 0x68
#define MPU_SMPL_RATE_DIV 0x19
#define MPU_CONFIG 0x1A
#define MPU_ACCEL_CONFIG 0x1C
#define MPU_ACCEL_XOUT_H 0x3B

#define MPU_ACCEL_RANGE_2G  0
#define MPU_ACCEL_RANGE_4G  1
#define MPU_ACCEL_RANGE_8G  2
#define MPU_ACCEL_RANGE_16G 3

#define MPU_ACCEL_RANGE_2G_LSB  16384
#define MPU_ACCEL_RANGE_4G_LSB  8192
#define MPU_ACCEL_RANGE_8G_LSB  4096
#define MPU_ACCEL_RANGE_16G_LSB 2048

void mpuReset()
{
    // Reset registers
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_PWR_MGMT_1);
    // PWR_MGMT_1[7] = Device Reset => 1
    // 10000000
    Wire.write(0x80);
    Wire.endTransmission();
    delay(100);

    // Reset signal path
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_SIG_PATH_RST);
    // SIG_PATH_RST[2:0] = Gyro Reset / Accel Reset / Temp Reset = 1
    // 00000111
    Wire.write(0x07);
    Wire.endTransmission();
    delay(100);
    Serial.println("MPU reset complete");
}

void mpuSetSampleDivider(int dividerMinus1)
{
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_SMPL_RATE_DIV);
    Wire.write(dividerMinus1);
    Wire.endTransmission();
}

void mpuSetLowPassFilter(byte lpfFrequencyCfg)
{
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_CONFIG);
    Wire.write(lpfFrequencyCfg);
    Wire.endTransmission();
}

void mpuSetAccelRange(byte rangeOption)
{
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_ACCEL_CONFIG);
    Wire.write(rangeOption << 3);
    Wire.endTransmission();
}

void mpuConfigPwrMgmt()
{
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_PWR_MGMT_1);

    // PWR_MGMT_1[2:0] = Clock Select => 1 (PLL referenced with Gyro X as opposed to internal oscillator)
    // PWR_MGMT_1[6] = Sleep => 0 (turn off sleep)
    Wire.write(1);
    Wire.endTransmission();
}

void mpuRead3AccelAxesRaw(int16_t *xAxisRaw, int16_t *yAxisRaw, int16_t *zAxisRaw, unsigned long *timeMs)
{
    // Point the register to the beginning of the accel out block
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_ACCEL_XOUT_H);
    Wire.endTransmission();

    // Read next 6 bytes (2 bytes per axis)
    Wire.requestFrom(MPU_ADDRESS, 6);
    uint8_t buffer[6];
    Wire.readBytes(buffer, 6);
    *xAxisRaw = buffer[0] << 8 | buffer[1];
    *yAxisRaw = buffer[2] << 8 | buffer[3];
    *zAxisRaw = buffer[4] << 8 | buffer[5];
    *timeMs = millis();
}

void mpuStart()
{
    // Reset registers
    mpuReset();
    // Set sample rate divisor to (0 + 1)
    mpuSetSampleDivider(0);

    // Set the Low Pass Filter to 260 Hz aka no filter
    // LPF_CFG 0 == 260 Hz aka no filter
    mpuSetLowPassFilter(3);
    // Set range of acceleromters to +- 2G (for now) will need 8 G
    mpuSetAccelRange(MPU_ACCEL_RANGE_2G);
    // Set clock to PLL with reference to X axis gyro and wake up the accelerometer
    mpuConfigPwrMgmt();
}

#endif