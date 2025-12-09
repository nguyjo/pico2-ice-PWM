#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ice_cram.h"
#include "ice_fpga.h"
#include "ice_led.h"

uint8_t bitstream[] = {
#include "bitstream.h"
};

int main(void) {
    stdio_init_all();
    ice_led_init();

    // Configure and start FPGA (program CRAM with your bitstream)
    ice_fpga_init(FPGA_DATA, 48); // 48 MHz for FPGA clock
    ice_fpga_start(FPGA_DATA);

    ice_cram_open(FPGA_DATA);
    ice_cram_write(bitstream, sizeof(bitstream));
    ice_cram_close();

    // Set up SPI1 (GPIO24–27)
    spi_init(spi1, 1000 * 1000); // 1 MHz
    gpio_set_function(24, GPIO_FUNC_SPI); // MISO
    gpio_set_function(27, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(26, GPIO_FUNC_SPI); // SCK
    gpio_set_function(25, GPIO_FUNC_SPI); // CS

    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST); // automatically handles Chip Select

    uint8_t txbuf[1] = {0x01};
    uint8_t rxbuf[1];

    while (true) {
        spi_write_read_blocking(spi1, txbuf, rxbuf, 1);
        // printf("Sent: 0x%02X, Received: 0x%02X\n", txbuf[0], rxbuf[0]);
        sleep_ms(500);

        // Toggle between 0x01 and 0x00
        txbuf[0] = (txbuf[0] == 0x00) ? 0x01 : 0x00;
    }
}
