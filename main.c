#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "ice_cram.h"
#include "ice_fpga.h"

// -------------------- FPGA CONFIG --------------------
#define FPGA_SPI       spi1
#define PIN_MISO       24
#define PIN_CS         25
#define PIN_SCK        26
#define PIN_MOSI       27
#define FPGA_CLK_FREQ  48 // MHz

// -------------------- IMU (LSM6DSOX) --------------------
#define LSM6_ADDR      0x6A
#define CTRL1_XL       0x10
#define CTRL2_G        0x11
#define CTRL3_C        0x12
#define OUTX_L_G       0x22
#define OUTX_L_A       0x28

// --- CRITICAL FIX: Pins 2 and 3 are I2C1 ---
#define IMU_I2C        i2c1  
#define IMU_SDA_PIN    2
#define IMU_SCL_PIN    3

// Bitstream
uint8_t bitstream[] = {
#include "bitstream.h"
};

// --- Helpers ---
static void i2c_write_u8(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    i2c_write_blocking(IMU_I2C, LSM6_ADDR, buf, 2, false);
}

static void read3_imu(uint8_t reg0, int16_t *x, int16_t *y, int16_t *z) {
    uint8_t buffer[6];
    i2c_write_blocking(IMU_I2C, LSM6_ADDR, &reg0, 1, true);
    i2c_read_blocking(IMU_I2C, LSM6_ADDR, buffer, 6, false);
    
    *x = (int16_t)((buffer[1] << 8) | buffer[0]);
    *y = (int16_t)((buffer[3] << 8) | buffer[2]);
    *z = (int16_t)((buffer[5] << 8) | buffer[4]);
}

uint8_t map_tilt_to_servo(float tilt) {
    float min_in = -45.0f;
    float max_in = 45.0f;
    if (tilt < min_in) tilt = min_in;
    if (tilt > max_in) tilt = max_in;
    return (uint8_t)((tilt - min_in) * 180.0f / (max_in - min_in));
}

int main() {
    stdio_init_all();
    sleep_ms(100);

    // 1. Initialize FPGA
    ice_fpga_init(FPGA_DATA, FPGA_CLK_FREQ); 
    ice_fpga_start(FPGA_DATA);
    ice_cram_open(FPGA_DATA);
    ice_cram_write(bitstream, sizeof(bitstream));
    ice_cram_close();

    // 2. Init I2C
    i2c_init(IMU_I2C, 400 * 1000);
    gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA_PIN);
    gpio_pull_up(IMU_SCL_PIN);

    // 3. Init SPI
    spi_init(FPGA_SPI, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    spi_set_format(FPGA_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // 4. Init IMU
    i2c_write_u8(CTRL3_C, 0x01); // Reset
    sleep_ms(50);
    i2c_write_u8(CTRL3_C, 0x44); // Block Data Update
    i2c_write_u8(CTRL2_G, 0x4C); // Gyro
    i2c_write_u8(CTRL1_XL, 0x4A); // Accel
    sleep_ms(50);

    float roll = 0.0f;
    float pitch = 0.0f;

    while (true) {
        int16_t axr, ayr, azr;
        int16_t gxr, gyr, gzr;
        
        // Read Sensors
        read3_imu(OUTX_L_A, &axr, &ayr, &azr);
        read3_imu(OUTX_L_G, &gxr, &gyr, &gzr);

        // Calculate Angles
        float accel_roll  = atan2f((float)ayr, (float)azr) * 180.0f / (float)M_PI;
        float accel_pitch = atan2f(-(float)axr, sqrtf((float)ayr*(float)ayr + (float)azr*(float)azr)) * 180.0f / (float)M_PI;

        // Filter
        roll  = 0.9f * roll  + 0.1f * accel_roll;
        pitch = 0.9f * pitch + 0.1f * accel_pitch;

        // --- MAP TO FPGA PROTOCOL ---
        // Get the base 0-180 value
        uint8_t raw_x = map_tilt_to_servo(roll);
        uint8_t raw_y = map_tilt_to_servo(pitch);

        // --- INVERT LOGIC HERE ---
        // If turning Right makes it go Left, we do (180 - value) to flip it.
        // If one axis is correct and the other is wrong, only do math on the wrong one.
        
        uint8_t tx_buf[2];

        // Invert X (Roll)
        tx_buf[0] = 180 - raw_x; 

        // Invert Y (Pitch)
        tx_buf[1] = 180 - raw_y;

        gpio_put(PIN_CS, 0);
        spi_write_blocking(FPGA_SPI, tx_buf, 2);
        gpio_put(PIN_CS, 1);

        sleep_ms(20);
    }
}