module spi_slave (
    input  logic rst_n,   // active-low reset
    input  logic sck,     // SPI clock from master
    input  logic cs_n,    // chip select (active low)
    input  logic mosi,    // master out, slave in
    output logic miso,    // master in, slave out
    output logic [7:0] data_out,
    output logic        rx_valid
);

    logic [7:0] shift_reg;
    logic [2:0] bit_cnt;

    // Assumes SPI mode 0: sample MOSI on rising edge of sck
    always_ff @(posedge sck or negedge rst_n) begin
        if (!rst_n) begin
            shift_reg <= 8'd0;
            bit_cnt   <= 3'd0;
            data_out  <= 8'd0;
            rx_valid  <= 1'b0;
        end else if (!cs_n) begin
            shift_reg <= {shift_reg[6:0], mosi};
            bit_cnt   <= bit_cnt + 1;

            if (bit_cnt == 3'd7) begin
                data_out <= {shift_reg[6:0], mosi};
                rx_valid <= 1'b1;
                bit_cnt  <= 3'd0;
            end else begin
                rx_valid <= 1'b0;
            end
        end else begin
            bit_cnt  <= 3'd0;
            rx_valid <= 1'b0;
        end
    end

    assign miso = shift_reg[7]; // simple loopback

endmodule
