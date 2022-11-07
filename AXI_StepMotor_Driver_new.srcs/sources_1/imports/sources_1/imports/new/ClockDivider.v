`timescale 1ns / 1ps

module ClockDivider(
    input i_clk,
    input [31:0] i_time,
    input i_reset,

    output o_time_clk
    );

    reg r_time_clk = 0;
    reg [31:0] r_counter = 0;

    assign o_time_clk = r_time_clk;

    always @(posedge i_clk or posedge i_reset) begin
        if (!i_reset || !i_time)begin
            r_time_clk <= 1'b0;
            r_counter <= 0;
        end
        else begin
            if (r_counter == 12207*i_time -1) begin
                r_counter <= 0;
                r_time_clk <= ~r_time_clk;
            end
            else begin
                r_counter <= r_counter + 1;
            end
        end
    end
endmodule
