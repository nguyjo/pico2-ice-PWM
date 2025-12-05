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

    // Optional: status LED for sanity
    ice_led_init();

    // Configure and start FPGA (program CRAM with your bitstream)
    ice_fpga_init(FPGA_DATA, 48);   // board-specific; keep as in your working loader
    ice_fpga_start(FPGA_DATA);

    ice_cram_open(FPGA_DATA);
    ice_cram_write(bitstream, sizeof(bitstream));
    ice_cram_close();

    // Now the FPGA is running your HDL; set up SPI1 (pins 24/25/26/27 on Pico)
    spi_init(spi1, 1000 * 1000); // 1 MHz

    gpio_set_function(24, GPIO_FUNC_SPI); // MISO (SPI 1)
    gpio_set_function(27, GPIO_FUNC_SPI); // MOSI (SPI 1)
    gpio_set_function(26, GPIO_FUNC_SPI); // SCK  (SPI 1)
    gpio_set_function(25, GPIO_FUNC_SPI); // CS   (SPI 1)

    // Mode 0 by default; ensure your slave samples on rising edge
    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    uint8_t txbuf[1] = {0xFF};
    uint8_t rxbuf[1];

    while (true) {
        spi_write_read_blocking(spi1, txbuf, rxbuf, 1);
        // Optionally print rxbuf[0] for debugging
        sleep_ms(500);
        txbuf[0] = (txbuf[0] == 0x00) ? 0xFF : 0x00;
    }
}
