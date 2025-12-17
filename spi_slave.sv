module spi_slave (
    input  logic CLK,
    input  logic sck,
    input  logic cs_n,
    input  logic mosi,
    output logic miso,
    output logic [15:0] data_out,
    output logic rx_valid
);
    logic [15:0] shift_reg;
    logic [4:0]  bit_cnt;

    always_ff @(posedge sck or posedge cs_n) begin
        if (cs_n) begin
            bit_cnt   <= 5'd0;
            rx_valid  <= 1'b0;
        end else begin
            // Implements SPI Shift Register (Buffer)
            shift_reg <= {shift_reg[14:0], mosi}; // Concatenate new MOSI bit into LSB in shift register Buffer
            bit_cnt   <= bit_cnt + 1;
            if (bit_cnt == 5'd15) begin
                data_out <= {shift_reg[14:0], mosi};
                rx_valid <= 1'b1;
                bit_cnt  <= 5'd0;
            end else begin
                rx_valid <= 1'b0;
            end
        end
    end
    assign miso = shift_reg[15];
endmodule