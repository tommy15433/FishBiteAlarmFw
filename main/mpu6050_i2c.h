#pragma once

#include "esp_err.h"

#define MPU6050_DATA_LEN 14
#define MPU6050_DEBUG_LEN 1

#define ACCEL_CONFIG    0x1C
#define AFS_SEL_2G      0x00
#define AFS_SEL_4G      0x08
#define AFS_SEL_8G      0x10
#define AFS_SEL_16G     0x18


typedef struct {
    union {
        uint8_t data[MPU6050_DATA_LEN];

        struct {
            short AX;
            short AY;
            short AZ;
            
            short Temp;

            short GyX;
            short GyY;
            short GyZ;
        } reg;
    };
} mpu6050_t;

int mpu6050_get_accel_resolution(void);

double mpu6050_get_accelX(void);
double mpu6050_get_accelY(void);
double mpu6050_get_accelZ(void);
double mpu6050_get_temp(void);
double mpu6050_get_gyroX(void);
double mpu6050_get_gyroY(void);
double mpu6050_get_gyroZ(void);

void mpu6050_deinit(void);
void mpu6050_read(mpu6050_t * this);
void mpu6050_update(void);
void mpu6050_sleep(void);
int mpu6050_init(void);

esp_err_t mpu6050_i2c_read_gx(int16_t* data);

