module spi_slave (
    input  logic CLK,     // System Clock
    input  logic sck,     // SPI Clock
    input  logic cs_n,    // Chip Select (Active Low)
    input  logic mosi,    // Master Out Slave In
    output logic miso,    // Master In Slave Out
    output logic [31:0] data_out, // 32-bit output (High 16: Servo1, Low 16: Servo2)
    output logic rx_valid
);

    logic [31:0] shift_reg;
    logic [5:0]  bit_cnt; // Need 6 bits to count 0-32

    // Capture data on SPI Clock rising edge, Reset on CS_N High
    always_ff @(posedge sck or posedge cs_n) begin
        if (cs_n) begin
            // Asynchronous Reset when CS is High
            bit_cnt   <= 6'd0;
            rx_valid  <= 1'b0;
        end else begin
            // Shift in bits (MSB First)
            shift_reg <= {shift_reg[30:0], mosi};
            bit_cnt   <= bit_cnt + 1;

            // When we have received 32 bits...
            if (bit_cnt == 6'd31) begin
                data_out <= {shift_reg[30:0], mosi};
                rx_valid <= 1'b1;
                bit_cnt  <= 6'd0;
            end else begin
                rx_valid <= 1'b0;
            end
        end
    end

    assign miso = shift_reg[31]; // Echo MSB back (optional)

endmodule