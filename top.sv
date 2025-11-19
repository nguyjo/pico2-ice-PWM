/* Blink the RGB LED */

module top (
    input  logic clock,
    output logic LED_R,
    output logic LED_G,
    output logic LED_B
);

    // Counter register with initialization
    logic [31:0] counter = '0;

    // Sequential logic: increment counter on each clock edge
    always_ff @(posedge clock) begin
        counter <= counter + 1;
    end

    // Drive outputs from counter bits
    // You can assign individually or as a group
    assign LED_R = counter[28];
    assign LED_G = counter[27];
    assign LED_B = counter[26];

endmodule
