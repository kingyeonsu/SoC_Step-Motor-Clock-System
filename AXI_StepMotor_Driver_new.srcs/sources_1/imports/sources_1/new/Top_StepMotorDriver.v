`timescale 1ns / 1ps

module Top_StepMotorDriver(
    input i_clk,
    input [31:0] i_time,
    input i_reset,
    input i_direction,

    output [3:0] o_step_motor
    );

    wire w_time_clk;
    wire [2:0] w_counter;

    ClockDivider U0(
        .i_clk(i_clk),
        .i_time(i_time),
        .i_reset(i_reset),
        .o_time_clk(w_time_clk)
    );

    StepCounter U1(
        .i_time_clk(w_time_clk),
        .i_reset(i_reset),
        .o_counter(w_counter)
    );

    Mux3x4 U2(
        .i_counter(w_counter),
        .i_direction(i_direction),
        .o_step_motor(o_step_motor)
    );

endmodule
