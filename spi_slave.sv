module spi_slave (
    input  logic CLK,     // system clock
    input  logic rst_n,   // active-low reset (We will ignore this in logic)
    input  logic sck,     // SPI clock from master
    input  logic cs_n,    // chip select (active low)
    input  logic mosi,    // master out, slave in
    output logic miso,    // master in, slave out
    output logic [15:0] data_out,
    output logic        rx_valid
);

    logic [15:0] shift_reg;
    logic [3:0]  bit_cnt;

    // FIX: Removed "negedge rst_n" from the sensitivity list.
    // We only trigger on SCK (Clock) or CS_N (Reset).
    always_ff @(posedge sck or posedge cs_n) begin
        if (cs_n) begin
            // Asynchronous Reset:
            // When CS is High (Inactive), force the bit counter to 0 immediately.
            // This fixes the synchronization bug.
            bit_cnt   <= 4'd0;
            rx_valid  <= 1'b0;
        end 
        else begin
            // Normal SPI operation (Sample on Rising Edge of SCK)
            shift_reg <= {shift_reg[14:0], mosi};
            bit_cnt   <= bit_cnt + 1;

            if (bit_cnt == 4'd15) begin 
                data_out <= {shift_reg[14:0], mosi};
                rx_valid <= 1'b1;
                bit_cnt  <= 4'd0; 
            end else begin
                rx_valid <= 1'b0;
            end
        end
    end

    assign miso = shift_reg[15];

endmodule