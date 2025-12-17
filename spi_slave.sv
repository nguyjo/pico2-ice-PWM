module spi_slave (
    input  logic CLK,     // System Clock
    input  logic sck,     // SPI Clock
    input  logic cs_n,    // Chip Select (Active Low)
    input  logic mosi,    // Master Out Slave In
    output logic miso,    // Master In Slave Out
    output logic [15:0] data_out, // CHANGED: 16-bit output
    output logic rx_valid
);

    logic [15:0] shift_reg; // CHANGED: 16-bit register
    logic [4:0]  bit_cnt;   // CHANGED: 5 bits is enough for 0-16

    // Capture data on SPI Clock rising edge, Reset on CS_N High
    always_ff @(posedge sck or posedge cs_n) begin
        if (cs_n) begin
            // Asynchronous Reset
            bit_cnt   <= 5'd0;
            rx_valid  <= 1'b0;
        end else begin
            // Shift in bits
            shift_reg <= {shift_reg[14:0], mosi}; // CHANGED: [14:0]
            bit_cnt   <= bit_cnt + 1;

            // Trigger when 16 bits received (0 to 15)
            if (bit_cnt == 5'd15) begin // CHANGED: Check for 15
                data_out <= {shift_reg[14:0], mosi};
                rx_valid <= 1'b1;
                bit_cnt  <= 5'd0;
            end else begin
                rx_valid <= 1'b0;
            end
        end
    end

    assign miso = shift_reg[15]; // CHANGED: Echo MSB 15

endmodule