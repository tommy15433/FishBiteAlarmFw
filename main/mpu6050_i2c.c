#include "mpu6050_i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#include "mpu6050_i2c.h"

static const char *TAG = "mpu6050_i2c";

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO 22               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUMBER(0) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define MPU6050_ADDR 0x68
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

typedef enum{
    REG_IMU = 0x02,
    REG_ACC_X_H = 0x3B,
    REG_ACC_X_L = 0x3C,
    REG_ACC_Y_H = 0x3D,
    REG_ACC_Y_L = 0x3E,
    REG_ACC_Z_H = 0x3F,
    REG_ACC_Z_L = 0x40,

    REG_GYRO_X_H = 0x43,
    REG_GYRO_X_L = 0x44,
    REG_GYRO_Y_H = 0x45,
    REG_GYRO_Y_L = 0x46,
    REG_GYRO_Z_H = 0x47,
    REG_GYRO_Z_L = 0x48,
    PWR_MGMT_1 = 0x6B,
    PWR_MGMT_2 = 0x6C,
} REG;

uint8_t data_register[MPU6050_DATA_LEN];

TaskHandle_t xHandle;
bool isReady = true;

/**
 * @brief i2c master initialization
 */



static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
   // conf.clk_flags = 0;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t i2c_read_byte(uint8_t addr, uint8_t* data)
{
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MPU6050_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, addr, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MPU6050_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    
    i2c_master_read_byte(cmd, data, NACK_VAL);
    
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t mpu6050_i2c_write(const REG reg, uint8_t data)
{
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MPU6050_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t mpu6050_i2c_read(const REG reg, uint8_t * buf)
{
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MPU6050_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)reg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MPU6050_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    for (int i = 0; i < MPU6050_DATA_LEN; i++) {
        if (i < MPU6050_DATA_LEN - 1) {
            i2c_master_read_byte(cmd, buf+i, ACK_VAL);
        } else {
            i2c_master_read_byte(cmd, buf+i, NACK_VAL);
        }
    }
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}


esp_err_t mpu6050_i2c_read_gx(int16_t* data)
{
    esp_err_t ret = ESP_OK;
    uint8_t h,l;

    ret = i2c_read_byte(0x43, &h);
    ret = i2c_read_byte(0x44, &l);

    *data = (h << 8) | l;
    
    return ret;
}

void mpu6050_deinit(void)
{
    gpio_config_t io_conf;
    i2c_driver_delete(I2C_MASTER_NUM);

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<I2C_MASTER_SDA_IO) | (1ULL<<I2C_MASTER_SCL_IO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
}

double mpu6050_get_accelX(){
    return (double)((data_register[0] << 8) | data_register[1]);
}
double mpu6050_get_accelY(){
    return (double)((data_register[2] << 8) | data_register[3]);
}
double mpu6050_get_accelZ(){
    return (double)((data_register[4] << 8) | data_register[5]);
}
double mpu6050_get_temp(){
    return (double)((data_register[6] << 8) | data_register[7]);
}
double mpu6050_get_gyroX(){
    return (double)((data_register[8] << 8) | data_register[9]);
}
double mpu6050_get_gyroY(){
    return (double)((data_register[10] << 8) | data_register[11]);
}
double mpu6050_get_gyroZ(){
    return (double)((data_register[12] << 8) | data_register[13]);
}

void mpu6050_update()
{
    esp_err_t err;
    err = mpu6050_i2c_read(REG_ACC_X_H, data_register);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: [mpu6050_i2c_read] fail. (%x)", __func__, err);
    }
}

void mpu6050_read(mpu6050_t * this)
{
    esp_err_t err;
    err = mpu6050_i2c_read(REG_ACC_X_H, this->data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: [mpu6050_i2c_read] fail. (%x)", __func__, err);
    }
}


void mpu6050_sleep(void)
{
    mpu6050_i2c_write(PWR_MGMT_1, 0x40);
    // mpu6050_i2c_write(PWR_MGMT_1, 0x21);
}

int accel_resolution = 16384;

int mpu6050_get_accel_resolution(void)
{
    uint8_t reg = 0x00;
        int resolution = 16384;
    mpu6050_i2c_read(ACCEL_CONFIG, &reg);
    
    switch ( reg & 0x18 )
    {
        case AFS_SEL_2G:
            resolution = 16384;
            break;
        case AFS_SEL_4G:
            resolution = 8192;
            break;
        case AFS_SEL_8G:
            resolution = 4096;
            break;
        case AFS_SEL_16G:
            resolution = 2048;
            break;
    }
    
    return resolution;
}

void mpu6050_init(void)
{
    esp_err_t err;
    
    err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "i2c master init err (%d)", err);
    }
    
    // err = mpu6050_i2c_write(PWR_MGMT_1, 0x20);
    // err = mpu6050_i2c_write(PWR_MGMT_2, 0x40);
    err = mpu6050_i2c_write(PWR_MGMT_1, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: [mpu6050_i2c_write] fail.", __func__);
    }

    err = mpu6050_i2c_write(ACCEL_CONFIG, AFS_SEL_16G);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: [mpu6050_i2c_write(0x1C, 0x18)] fail.", __func__);
    }
    // accel resolution 16G

}
