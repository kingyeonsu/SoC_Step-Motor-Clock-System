`timescale 1ns / 1ps

module Mux3x4(
    input [2:0] i_counter,
    input i_direction,
    output [3:0] o_step_motor
    );

    reg [3:0] r_step_motor;
    assign o_step_motor = r_step_motor;

    always @(*) begin
        if (i_direction) begin
            case (i_counter)
            3'b000 : r_step_motor = 4'b0001;
            3'b001 : r_step_motor = 4'b0011;
            3'b010 : r_step_motor = 4'b0010;
            3'b011 : r_step_motor = 4'b0110;
            3'b100 : r_step_motor = 4'b0100;
            3'b101 : r_step_motor = 4'b1100;
            3'b110 : r_step_motor = 4'b1000;
            3'b111 : r_step_motor = 4'b1001;
            default : r_step_motor = 4'b0000;
            endcase
        end
        else begin
            case (i_counter)
            3'b000 : r_step_motor = 4'b1001;
            3'b001 : r_step_motor = 4'b1000;
            3'b010 : r_step_motor = 4'b1100;
            3'b011 : r_step_motor = 4'b0100;
            3'b100 : r_step_motor = 4'b0110;
            3'b101 : r_step_motor = 4'b0010;
            3'b110 : r_step_motor = 4'b0011;
            3'b111 : r_step_motor = 4'b0001;
            default : r_step_motor = 4'b0000;
            endcase
        end
    end
endmodule
