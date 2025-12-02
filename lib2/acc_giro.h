#ifndef ACC_GIRO_H
#define ACC_GIRO_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

// Endereço I2C do MPU6050
#define I2C_PORT i2c0               // i2c0 nos pinos 0 e 1, i2c1 nos pinos 2 e 3
#define I2C_SDA 0                   // 0 ou 2
#define I2C_SCL 1                   // 1 ou 3

// O endereço padrão deste IMU é 0x68
static int addr = 0x68;

// Função para resetar o MPU6050
static void mpu6050_reset()
{
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
    sleep_ms(100);

    buf[1] = 0x00;
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false); 
    sleep_ms(10);
}

// Função para ler valores brutos do acelerômetro e giroscópio
static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3])
{
    uint8_t buffer[6];

    // Lê acelerômetro
    uint8_t val = 0x3B;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Lê giroscópio
    val = 0x43;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }
}

void iniciar_i2c_IMU()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    printf("Antes do bi_decl...\n");
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
    printf("Antes do reset MPU...\n");
    mpu6050_reset();
}

int16_t acceleration[3], gyro[3];



// Função utilitária para ler acelerômetro e giroscópio em arrays fornecidos (valores brutos)
static inline void ler_acc_giro(int16_t accel[3], int16_t gyro[3])
{
    mpu6050_read_raw(accel, gyro);
}

// Conversão dos valores brutos para unidades físicas
// Fatores padrão: acelerômetro ±2g (16384 LSB/g), giroscópio ±250°/s (131 LSB/(°/s))
#define MPU6050_ACCEL_SENS_2G 16384.0f
#define MPU6050_GYRO_SENS_250 131.0f
#define GRAVIDADE 9.80665f

// Converte valores brutos do acelerômetro para m/s^2
static inline void accel_raw_to_ms2(const int16_t accel_raw[3], float accel_ms2[3])
{
    for (int i = 0; i < 3; i++) {
        accel_ms2[i] = ((float)accel_raw[i] / MPU6050_ACCEL_SENS_2G) * GRAVIDADE;
    }
}

// Converte valores brutos do giroscópio para °/s
static inline void gyro_raw_to_dps(const int16_t gyro_raw[3], float gyro_dps[3])
{
    for (int i = 0; i < 3; i++) {
        gyro_dps[i] = (float)gyro_raw[i] / MPU6050_GYRO_SENS_250;
    }
}

void loop_leitura_IMU()
{
    mpu6050_read_raw(acceleration, gyro);

    printf("Acc. X = %d, Y = %d, Z = %d\n", acceleration[0], acceleration[1], acceleration[2]);
    printf("Gyro. X = %d, Y = %d, Z = %d\n", gyro[0], gyro[1], gyro[2]);
}

#endif
