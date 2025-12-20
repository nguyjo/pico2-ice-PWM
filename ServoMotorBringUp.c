#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "ice_cram.h"
#include "ice_fpga.h"

// --- CONFIGURATION ---
#define FPGA_CS_PIN 25      // Manual Chip Select
#define FPGA_CLK_FREQ 48    // We Init at 48MHz, Verilog handles the divider

// Servo Limits (Calculated for 24MHz effective speed)
// 0.6ms = 14400 ticks
// 2.4ms = 57600 ticks
#define SERVO_MIN 14400
#define SERVO_MAX 57600

uint8_t bitstream[] = {
#include "bitstream.h"
};

int main(void) {
    stdio_init_all();

    // 1. Initialize FPGA
    ice_fpga_init(FPGA_DATA, FPGA_CLK_FREQ); 
    ice_fpga_start(FPGA_DATA);
    ice_cram_open(FPGA_DATA);
    ice_cram_write(bitstream, sizeof(bitstream));
    ice_cram_close();

    // 2. Initialize SPI (1 MHz)
    spi_init(spi1, 1000 * 1000); 
    gpio_set_function(24, GPIO_FUNC_SPI); // MISO
    gpio_set_function(27, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(26, GPIO_FUNC_SPI); // SCK

    // Manual Chip Select
    gpio_init(FPGA_CS_PIN);
    gpio_set_dir(FPGA_CS_PIN, GPIO_OUT);
    gpio_put(FPGA_CS_PIN, 1); 

    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    while (true) {
        // --- MOVE TO 0 DEGREES (MIN) ---
        uint16_t s1 = SERVO_MIN;
        uint16_t s2 = SERVO_MIN;
        
        // Pack 32 bits: [S1 High] [S1 Low] [S2 High] [S2 Low]
        uint8_t tx_buf[4];
        tx_buf[0] = (s1 >> 8) & 0xFF;
        tx_buf[1] = (s1 & 0xFF);
        tx_buf[2] = (s2 >> 8) & 0xFF;
        tx_buf[3] = (s2 & 0xFF);

        gpio_put(FPGA_CS_PIN, 0); // CS Low
        spi_write_blocking(spi1, tx_buf, 4); // Send 4 bytes
        gpio_put(FPGA_CS_PIN, 1); // CS High
        
        sleep_ms(1000);

        // --- MOVE TO 180 DEGREES (MAX) ---
        s1 = SERVO_MAX;
        s2 = SERVO_MAX;

        tx_buf[0] = (s1 >> 8) & 0xFF;
        tx_buf[1] = (s1 & 0xFF);
        tx_buf[2] = (s2 >> 8) & 0xFF;
        tx_buf[3] = (s2 & 0xFF);

        gpio_put(FPGA_CS_PIN, 0); 
        spi_write_blocking(spi1, tx_buf, 4); 
        gpio_put(FPGA_CS_PIN, 1); 

        sleep_ms(1000);
    }
}