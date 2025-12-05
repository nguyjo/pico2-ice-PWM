#include "pico/stdlib.h"
#include "hardware/spi.h"

int main() {
    stdio_init_all();

    // Init SPI0 at 1 MHz
    spi_init(spi0, 1000 * 1000);

    // Configure GPIO pins for SPI0
    gpio_set_function(16, GPIO_FUNC_SPI); // MISO
    gpio_set_function(19, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(18, GPIO_FUNC_SPI); // SCK
    gpio_set_function(17, GPIO_FUNC_SPI); // CS

    uint8_t txbuf[1] = {0xFF}; // send one byte
    uint8_t rxbuf[1];

    while (true) {
        spi_write_read_blocking(spi0, txbuf, rxbuf, 1);
        sleep_ms(500);

        // Toggle between 0xFF and 0x00
        txbuf[0] = (txbuf[0] == 0x00) ? 0xFF : 0x00;
    }
}
