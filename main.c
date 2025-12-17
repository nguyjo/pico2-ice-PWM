#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// -------------------------------
// LSM6DSOX I2C + registers
// -------------------------------
#define LSM6_ADDR     0x6A

#define CTRL1_XL      0x10
#define CTRL2_G       0x11
#define CTRL3_C       0x12

#define OUTX_L_G      0x22
#define OUTX_L_A      0x28

// Sensitivities (approx, matches your MicroPython constants)
#define ACC_LSB_TO_G        (0.000061f)  // 2g range ~0.061 mg/LSB
#define GYRO_LSB_TO_DPS     (0.07f)      // 2000 dps range ~70 mdps/LSB

static inline float rad2deg(float r) { return r * (180.0f / (float)M_PI); }

// -------------------------------
// I2C helpers
// -------------------------------
static bool lsm6_write_u8(i2c_inst_t *i2c, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return i2c_write_blocking(i2c, LSM6_ADDR, buf, 2, false) == 2;
}

static bool lsm6_read(i2c_inst_t *i2c, uint8_t reg, uint8_t *dst, size_t n) {
    // Write register address, then read
    if (i2c_write_blocking(i2c, LSM6_ADDR, &reg, 1, true) != 1) return false;
    return i2c_read_blocking(i2c, LSM6_ADDR, dst, n, false) == (int)n;
}

static int16_t s16(uint8_t lo, uint8_t hi) {
    return (int16_t)((hi << 8) | lo);
}

static bool lsm6_read3_s16(i2c_inst_t *i2c, uint8_t reg0, int16_t *x, int16_t *y, int16_t *z) {
    uint8_t d[6];
    if (!lsm6_read(i2c, reg0, d, 6)) return false;
    *x = s16(d[0], d[1]);
    *y = s16(d[2], d[3]);
    *z = s16(d[4], d[5]);
    return true;
}

// -------------------------------
// Main
// -------------------------------
int main() {
    stdio_init_all();
    sleep_ms(1500); // give USB serial time

    // I2C1 on GP2(SDA) and GP3(SCL)
    i2c_inst_t *I2C = i2c1;
    i2c_init(I2C, 10 * 1000); // 10 kHz like your MicroPython
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);

    printf("\nLSM6DSOX roll/pitch + gyro test (Pico SDK C)\n");

    // -------------------------------
    // Init sensor
    // -------------------------------
    // Reset
    lsm6_write_u8(I2C, CTRL3_C, 0x01);
    sleep_ms(50);

    // IF_INC=1 (bit2), BDU=1 (bit6)
    lsm6_write_u8(I2C, CTRL3_C, (1u << 2) | (1u << 6));
    sleep_ms(10);

    // Gyro: ODR=104 Hz, FS=2000 dps
    lsm6_write_u8(I2C, CTRL2_G, 0x4C);
    sleep_ms(10);

    // Accel: ODR=104 Hz, FS=2g
    lsm6_write_u8(I2C, CTRL1_XL, 0x40);
    sleep_ms(50);

    // -------------------------------
    // Calibration: gyro bias (keep still)
    // -------------------------------
    printf("Calibrating gyro bias... keep IMU still on the table (~3s)\n");

    const int samples = 60;
    int32_t sum_gx = 0, sum_gy = 0, sum_gz = 0;

    for (int i = 0; i < samples; i++) {
        int16_t gx, gy, gz;
        if (!lsm6_read3_s16(I2C, OUTX_L_G, &gx, &gy, &gz)) {
            printf("I2C read failed during calibration!\n");
            sleep_ms(200);
            continue;
        }
        sum_gx += gx;
        sum_gy += gy;
        sum_gz += gz;
        sleep_ms(50);
    }

    float bias_gx = (float)sum_gx / (float)samples;
    float bias_gy = (float)sum_gy / (float)samples;
    float bias_gz = (float)sum_gz / (float)samples;

    printf("Gyro bias (raw): %.2f %.2f %.2f\n", bias_gx, bias_gy, bias_gz);
    printf("Printing roll/pitch degrees every 0.2s\n\n");

    // -------------------------------
    // Loop
    // -------------------------------
    while (true) {
        int16_t ax_raw, ay_raw, az_raw;
        int16_t gx_raw, gy_raw, gz_raw;

        if (!lsm6_read3_s16(I2C, OUTX_L_A, &ax_raw, &ay_raw, &az_raw) ||
            !lsm6_read3_s16(I2C, OUTX_L_G, &gx_raw, &gy_raw, &gz_raw)) {
            printf("I2C read failed!\n");
            sleep_ms(200);
            continue;
        }

        // Convert accel to g
        float ax = (float)ax_raw * ACC_LSB_TO_G;
        float ay = (float)ay_raw * ACC_LSB_TO_G;
        float az = (float)az_raw * ACC_LSB_TO_G;

        // Compute roll/pitch from gravity
        float roll  = rad2deg(atan2f(ay, az));
        float pitch = rad2deg(atan2f(-ax, sqrtf(ay*ay + az*az)));

        // Gyro deg/s with bias removed
        float gx_dps = ((float)gx_raw - bias_gx) * GYRO_LSB_TO_DPS;
        float gy_dps = ((float)gy_raw - bias_gy) * GYRO_LSB_TO_DPS;
        float gz_dps = ((float)gz_raw - bias_gz) * GYRO_LSB_TO_DPS;

        printf("roll_deg: %7.2f  pitch_deg: %7.2f   |   gyro_dps: %7.2f %7.2f %7.2f\n",
               roll, pitch, gx_dps, gy_dps, gz_dps);

        sleep_ms(200);
    }
}
