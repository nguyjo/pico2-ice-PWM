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
    input  logic CLK,    // 48 MHz
    input  logic sck, mosi, cs_n,
    output logic miso,
    output logic ICE_46, // SERVO X
    output logic ICE_48, // SERVO Y
    output logic ICE_47 // LASER
);

    // --- 1. LASER ALWAYS ON ---
    // No SPI control needed. Just turn it on.
    assign ICE_47 = 1'b1;

    // --- 2. SPI INTERFACE (16 Bits) ---
    logic [15:0] spi_data;
    logic rx_valid;
    
    // Registers for Angles (0-180)
    logic [7:0] angle_x;
    logic [7:0] angle_y;

    // Use a standard 16-bit SPI Slave
    spi_slave u_spi (
        .CLK(CLK), .sck(sck), .mosi(mosi), .cs_n(cs_n), .miso(miso),
        .data_out(spi_data), .rx_valid(rx_valid)
    );

    always_ff @(posedge CLK) begin
        if (rx_valid) begin
            angle_x <= spi_data[15:8]; // First Byte
            angle_y <= spi_data[7:0];  // Second Byte
        end
    end

    // --- 3. THE CALCULATOR (Degrees -> Ticks) ---
    // Formula: Ticks = 14400 + (Angle * 240)
    logic [15:0] width_x;
    logic [15:0] width_y;

    always_ff @(posedge CLK) begin
        width_x <= 16'd14400 + (angle_x * 16'd240);
        width_y <= 16'd14400 + (angle_y * 16'd240);
    end

    // --- 4. PRESCALER (48MHz -> 24MHz) ---
    logic tick_24mhz;
    logic clk_div;
    always_ff @(posedge CLK) begin
        clk_div <= ~clk_div;
        tick_24mhz <= (clk_div == 0);
    end

    // --- 5. PWM GENERATOR ---
    logic [18:0] pwm_cnt;
    always_ff @(posedge CLK) begin
        if (tick_24mhz) begin
            if (pwm_cnt >= 479999) pwm_cnt <= 0;
            else pwm_cnt <= pwm_cnt + 1;
        end
    end

    assign ICE_46 = (pwm_cnt < width_x);
    assign ICE_48 = (pwm_cnt < width_y);

endmodule