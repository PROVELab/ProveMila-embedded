#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <TaskScheduler.h>
#include "mpu6050.h"

// ==== Test Parameters ====
#define SAMPLE_RATE_HZ 500UL
#define SAMPLE_DELAY_MS 30UL * 1000UL
#define SAMPLE_WINDOW_MS 60UL * 1000UL
#define SAMPLE_PERIOD_MS (1000UL / SAMPLE_RATE_HZ)
#define N_DECIMALS 2

#define N_SAMPLES ((unsigned long) (SAMPLE_WINDOW_MS / SAMPLE_PERIOD_MS))

// ==== Calibration Parameters ====
#define X_ACCEL_OFFSET 481
#define Y_ACCEL_OFFSET -85
#define Z_ACCEL_OFFSET 79

// ==== SD Parameters ====
#define SD_CS 10

// ==== MPU Parameters ====
#define MPU_ACCEL_RANGE MPU_ACCEL_RANGE_8G
#define MPU_ACCEL_RANGE_LSB MPU_ACCEL_RANGE_8G_LSB

File logFile;

void getSample();
bool getSampleOnEnable();
void getSampleOnDisable();

Scheduler runner;
Task getSampleTask(SAMPLE_PERIOD_MS, N_SAMPLES, getSample);

void writeAccelValuesToSD(float xAccelG, float yAccelG, float zAccelG, unsigned long time)
{
    char xAccelStr[8];
    dtostrf(xAccelG, 3, N_DECIMALS, xAccelStr);
    char yAccelStr[8];
    dtostrf(yAccelG, 3, N_DECIMALS, yAccelStr);
    char zAccelStr[8];
    dtostrf(zAccelG, 3, N_DECIMALS, zAccelStr);

    char csvBuf[50];
    snprintf(csvBuf, 50, "%u,%s,%s,%s", (unsigned int) time, xAccelStr, yAccelStr, zAccelStr);

    logFile.println(csvBuf);

}

void getSample()
{
    int16_t xAccelRaw;
    int16_t yAccelRaw;
    int16_t zAccelRaw;
    unsigned long timeMillis;

    mpuRead3AccelAxesRaw(&xAccelRaw, &yAccelRaw, &zAccelRaw, &timeMillis);

    float xAccelG = (float) (xAccelRaw + X_ACCEL_OFFSET) / MPU_ACCEL_RANGE_LSB;
    float yAccelG = (float) (yAccelRaw + Y_ACCEL_OFFSET) / MPU_ACCEL_RANGE_LSB;
    float zAccelG = (float) (zAccelRaw + Z_ACCEL_OFFSET) / MPU_ACCEL_RANGE_LSB;

    writeAccelValuesToSD(xAccelG, yAccelG, zAccelG, timeMillis);
}

bool getSampleOnEnable()
{
    Serial.print("Sampling ");
    Serial.print(N_SAMPLES);
    Serial.print(" samples in ");
    Serial.print(SAMPLE_WINDOW_MS);
    Serial.println(" ms");

    logFile = SD.open("accel.csv", O_CREAT | O_TRUNC | O_WRITE);
    return true;
}

void getSampleOnDisable()
{
    Serial.println("Finished sampling");
    logFile.close();
}

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    Wire.setClock(400E3);
    mpuStart();
    mpuSetAccelRange(MPU_ACCEL_RANGE);

    if (!SD.begin(SD_CS))
    {
        Serial.println("Unable to connect to SD card");
        while (1);
    }

    getSampleTask.setOnEnable(getSampleOnEnable);
    getSampleTask.setOnDisable(getSampleOnDisable);

    runner.addTask(getSampleTask);
    getSampleTask.enableDelayed(SAMPLE_DELAY_MS);
}

void loop()
{
    runner.execute();
}
