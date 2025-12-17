//////////////////////////////////////////////////////////////////////////////////
// Engineer: Joseph Nguyen and Andrei Vasilev
//
// Create Date: 11/18/2026
// Design Name: 
// Module Name: top
// Project Name: SPI Interface and PWM Signal Generator
// Target Devices: pico2-ice
// Tool versions:
// Description: Sends a PWM pulse up to the compare value on a 400 hz refresh
//
// Dependencies:
//
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// Based off code examples from
// embbededmicro.com/tutorials/mojo/pulse-width-modulation
//
//////////////////////////////////////////////////////////////////////////////////

module top (
    input  logic CLK,      // 48 MHz Clock
    input  logic sck,
    input  logic mosi,
    input  logic cs_n,
    output logic miso,
    output logic ICE_48,
    output logic ICE_46
);

    // --- SPI INTERFACE ---
    logic [31:0] spi_data;
    logic rx_valid;
    
    // Registers to hold pulse widths
    logic [15:0] width_1; // Servo 1 (High 16 bits)
    logic [15:0] width_2; // Servo 2 (Low 16 bits)

    spi_slave u_spi (
        .CLK(CLK),
        .sck(sck), .mosi(mosi), .cs_n(cs_n), .miso(miso),
        .data_out(spi_data),
        .rx_valid(rx_valid)
    );

    // Latch data when valid packet arrives
    always_ff @(posedge CLK) begin
        if (rx_valid) begin
            width_1 <= spi_data[31:16];
            width_2 <= spi_data[15:0];
        end
    end

    // --- 24 MHz PRESCALER ---
    // The FPGA runs at 48MHz. We toggle a bit to create a "tick" every 2 cycles.
    // This makes our math effective 24MHz.
    logic tick_24mhz;
    logic [1:0] div_cnt;
    always_ff @(posedge CLK) begin
        div_cnt <= div_cnt + 1;
        tick_24mhz <= (div_cnt[0] == 1'b1);
    end

    // --- PWM GENERATOR ---
    // 20ms period @ 24MHz = 480,000 ticks
    logic [18:0] pwm_cnt; 

    always_ff @(posedge CLK) begin
        if (tick_24mhz) begin
            if (pwm_cnt >= 479999) 
                pwm_cnt <= 0;
            else 
                pwm_cnt <= pwm_cnt + 1;
        end
    end

    // Output Logic
    assign ICE_48 = (pwm_cnt < width_1) ? 1'b1 : 1'b0;
    assign ICE_46 = (pwm_cnt < width_2) ? 1'b1 : 1'b0;

endmodule