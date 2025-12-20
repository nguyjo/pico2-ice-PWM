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

module LaserDiodeBringUp (
    input  logic CLK,    // 48 MHz clock input
    output logic LED_G,  // Blue LED output (active-low)
    output logic ICE_46  // Laser Diode output (active-low)
);

    // Clock and blink frequency parameters
    localparam int CLOCK_FREQUENCY = 48_000_000; // 48 MHz
    localparam int BLINK_FREQUENCY = 1;          // 1 Hz

    // Counter limit for half-period
    localparam int COUNTER_MAX = CLOCK_FREQUENCY / (2 * BLINK_FREQUENCY);

    // Counter register
    logic [$clog2(COUNTER_MAX)-1:0] counter = '0;

    // LED toggle logic
    always_ff @(posedge CLK) begin
        if (counter == COUNTER_MAX - 1) begin
            counter <= '0;
            LED_G  <= ~LED_G;   // Toggle LED
            ICE_46 <= ~ICE_46; // Toggle Laser Diode
        end else begin
            counter <= counter + 1;
        end
    end

    // Initialize output
    initial LED_G = 1'b1; // LED off initially (active-low)
    initial ICE_46 = 1'b0; // Laser Diode off initially (active-high)

endmodule