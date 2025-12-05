//////////////////////////////////////////////////////////////////////////////////
// Engineer: Joseph Nguyen and Andrei Vasilev
//
// Create Date: 11/18/2026
// Design Name: Digital Controller for VTOL UAV
// Module Name: pwm
// Project Name:
// Target Devices: Mojov V3 Spartan 6 xclx9
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
    // input  logic rst,
    input  logic sck,
    input  logic mosi,
    input  logic cs_n,
    output logic miso,
    output logic LED_G
);

    logic [7:0] data_out;
    logic       rx_valid;

    spi_slave u_spi (
        .rst_n(1'b1),   // permanently deeassert reset
        .sck(sck),
        .mosi(mosi),
        .cs_n(cs_n),
        .miso(miso),
        .data_out(data_out),
        .rx_valid(rx_valid)
    );

    // Drive LED: ON if received byte is nonzero
    assign LED_G = |data_out;

endmodule
