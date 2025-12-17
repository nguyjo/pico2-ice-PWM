#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "ice_cram.h"
#include "ice_fpga.h"

// -------------------- FPGA CONFIG --------------------
// The pico2-ice connects RP2350 to FPGA via SPI1
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

// Your requested I2C Pins
#define IMU_I2C        i2c0
#define IMU_SDA_PIN    2
#define IMU_SCL_PIN    3

// Bitstream for the FPGA
uint8_t bitstream[] = {
#include "bitstream.h"
};

// --- Low level I2C helpers ---
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

// --- Mapping Helper ---
// Maps physical tilt (-45 to +45) to Servo Angle (0 to 180)
uint8_t map_tilt_to_servo(float tilt) {
    float min_in = -45.0f;
    float max_in = 45.0f;
    
    if (tilt < min_in) tilt = min_in;
    if (tilt > max_in) tilt = max_in;

    // Linear map to 0..180
    return (uint8_t)((tilt - min_in) * 180.0f / (max_in - min_in));
}

int main() {
    stdio_init_all();
    sleep_ms(100); // Allow power to stabilize

    // 1. Initialize FPGA
    ice_fpga_init(FPGA_DATA, FPGA_CLK_FREQ); 
    ice_fpga_start(FPGA_DATA);
    ice_cram_open(FPGA_DATA);
    ice_cram_write(bitstream, sizeof(bitstream));
    ice_cram_close();

    // 2. --- I2C INITIALIZATION ---
    i2c_init(IMU_I2C, 400 * 1000); // 400kHz
    gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA_PIN);
    gpio_pull_up(IMU_SCL_PIN);

    // 3. --- SPI INITIALIZATION ---
    spi_init(FPGA_SPI, 1000 * 1000); // 1 MHz
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Manual Chip Select
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // Deselect

    spi_set_format(FPGA_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // 4. --- IMU SETUP ---
    // Reset
    i2c_write_u8(CTRL3_C, 0x01); 
    sleep_ms(50);
    // Block Data Update
    i2c_write_u8(CTRL3_C, 0x44); 
    // Gyro: 104Hz, 2000dps
    i2c_write_u8(CTRL2_G, 0x4C); 
    // Accel: 104Hz, 4g
    i2c_write_u8(CTRL1_XL, 0x4A); 
    sleep_ms(50);

    // Accel sensitivity for +/- 4g range is approx 0.122 mg/LSB
    // Standard gravity math uses raw values directly usually
    
    // Variables for Sensor Fusion
    float roll = 0.0f;
    float pitch = 0.0f;

    while (true) {
        int16_t axr, ayr, azr;
        int16_t gxr, gyr, gzr;
        
        read3_imu(OUTX_L_A, &axr, &ayr, &azr);
        read3_imu(OUTX_L_G, &gxr, &gyr, &gzr);

        // --- MATH: Calculate Pitch & Roll ---
        // 1. Convert Accel to Angles (in Degrees)
        // Note: We cast to float to ensure floating point math
        float accel_roll  = atan2f((float)ayr, (float)azr) * 180.0f / (float)M_PI;
        float accel_pitch = atan2f(-(float)axr, sqrtf((float)ayr*(float)ayr + (float)azr*(float)azr)) * 180.0f / (float)M_PI;

        // 2. Simple Sensor Fusion (Complementary Filter)
        // Keeps it smooth. 90% previous value, 10% new accel value.
        // (If you want to use the Gyro integration, you can add that back, 
        // but Accel-only is often enough for a simple pointer test)
        roll  = 0.9f * roll  + 0.1f * accel_roll;
        pitch = 0.9f * pitch + 0.1f * accel_pitch;

        // --- MAP TO FPGA PROTOCOL ---
        // Top.sv expects 0-180 for Servo X and Servo Y
        uint8_t servo_x = map_tilt_to_servo(roll);
        uint8_t servo_y = map_tilt_to_servo(pitch);

        // --- SEND SPI (2 BYTES) ---
        uint8_t tx_buf[2];
        tx_buf[0] = servo_x; 
        tx_buf[1] = servo_y;

        gpio_put(PIN_CS, 0);
        spi_write_blocking(FPGA_SPI, tx_buf, 2);
        gpio_put(PIN_CS, 1);

        // Debug output
        // printf("R: %.1f P: %.1f -> X: %d Y: %d\n", roll, pitch, servo_x, servo_y);

        sleep_ms(20); // 50Hz Update Rate
    }
}