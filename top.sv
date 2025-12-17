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
    output logic ICE_47  // LASER
);

    assign ICE_47 = 1'b1; // Laser Always On

    // SPI Interface
    logic [15:0] spi_data;
    // We ignore rx_valid from slave, we use CS_N logic instead
    logic unused_valid;

    spi_slave u_spi (
        .CLK(CLK), .sck(sck), .mosi(mosi), .cs_n(cs_n), .miso(miso),
        .data_out(spi_data), .rx_valid(unused_valid)
    );

    // --- ROBUST SPI LATCHING ---
    // We sample CS_N using the fast system clock (48MHz).
    // When CS_N goes from Low (0) to High (1), we grab the data.
    logic [1:0] cs_sync;
    logic [7:0] angle_x;
    logic [7:0] angle_y;

    always_ff @(posedge CLK) begin
        cs_sync <= {cs_sync[0], cs_n}; // Shift in CS state
        
        // If CS transitions from 0 to 1 (01 binary)
        if (cs_sync == 2'b01) begin
            angle_x <= spi_data[15:8];
            angle_y <= spi_data[7:0];
        end
    end

    // --- MATH & PWM ---
    logic [15:0] width_x;
    logic [15:0] width_y;

    always_ff @(posedge CLK) begin
        width_x <= 16'd14400 + (angle_x * 16'd240);
        width_y <= 16'd14400 + (angle_y * 16'd240);
    end

    // 24 MHz Prescaler
    logic tick_24mhz;
    logic clk_div;
    always_ff @(posedge CLK) begin
        clk_div <= ~clk_div;
        tick_24mhz <= (clk_div == 0);
    end

    // PWM Counter
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