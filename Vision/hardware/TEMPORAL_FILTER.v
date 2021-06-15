module TEMPORAL_FILTER(
	input                         clk,
	input                         rst,
	input	     [7:0]            r_in,
	input	     [7:0]            g_in,
	input	     [7:0]            b_in,
    input        [10:0]           x_in,
    input        [10:0]           y_in,
	output reg 	 [7:0] 			  r_out,
	output reg   [7:0] 			  g_out,
	output reg   [7:0] 			  b_out
);

/*
    This module implements a temporal moving average filter of the image frame.
    It saves the last n frames, adds the last n pixels at each x,y position, then gets the mean by division by n.
    This should reduce the noise that is present in the image and should ideally improve the performance of the image processor.
*/

reg [23:0] frame_history [480-1:0][640-1:0][2:0];

always @(posedge clk) begin
    // Shift the previous pixel values through the array
    frame_history[x_in][y_in][2] <= frame_history[x_in][y_in][1];
    frame_history[x_in][y_in][1] <= frame_history[x_in][y_in][0];
    frame_history[x_in][y_in][0] <= {r_in,g_in,b_in};

    // Calculate the moving average of the current pixel coordinate
    r_out <= (frame_history[x_in][y_in][2][7:0] + frame_history[x_in][y_in][1][7:0] + frame_history[x_in][y_in][0][7:0] + r_in ) >> 2;
    g_out <= (frame_history[x_in][y_in][2][15:8] + frame_history[x_in][y_in][1][15:8] + frame_history[x_in][y_in][0][15:8] + g_in ) >> 2;
    b_out <= (frame_history[x_in][y_in][2][23:16] + frame_history[x_in][y_in][1][23:16] + frame_history[x_in][y_in][0][23:16] + b_in ) >> 2;
end

endmodule
