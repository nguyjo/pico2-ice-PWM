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
    input  logic sck,
    input  logic mosi,
    input  logic cs_n,
    output logic miso,
    output logic LED_G
);

    logic [7:0] data_out;
    logic       rx_valid;

    // Optional: simple power-on reset to known state
    logic [7:0] por_cnt = '0;
    logic       rst_n;
    always_ff @(posedge sck or posedge cs_n) begin
        if (cs_n) begin
            por_cnt <= '0;
        end else if (por_cnt != 8'hFF) begin
            por_cnt <= por_cnt + 1;
        end
    end
    assign rst_n = (por_cnt == 8'hFF);

    spi_slave u_spi (
        .rst_n(rst_n),      // or 1'b1 if you prefer no POR
        .sck(sck),
        .mosi(mosi),
        .cs_n(cs_n),
        .miso(miso),
        .data_out(data_out),
        .rx_valid(rx_valid)
    );

    // LED on if any bit in received byte is 1
    assign LED_G = |data_out;

endmodule
