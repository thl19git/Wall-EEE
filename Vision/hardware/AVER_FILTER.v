module AVER_FILTER(
	input                         clk,
	input                         rst,
    input                         valid,
	input 					      packet_video,
	input	     [7:0]            r_in,
	input	     [7:0]            g_in,
	input	     [7:0]            b_in,
    input        [10:0]           x_in,
    input        [10:0]           y_in,
	output reg   [7:0] 			  r_out,
	output reg   [7:0] 			  g_out,
	output reg   [7:0] 			  b_out
);

/*
	This filter stores the previous two pixels in a register
	and	then calculates an average output over three consecutive pixels.
	This means that the filtering operation only takes place in the x-direction
	and therefore makes the filter logic much simpler due to the nature in which
	the pixels arrive.
*/


reg [23:0] prev_pixel [1:0];

always @(posedge clk) begin

	if (~packet_video) begin
		r_out <= r_in;
		g_out <= g_in;
		b_out <= b_in;
	end
	else if(valid) begin
		prev_pixel[1] <= prev_pixel[0];
		prev_pixel[0] <= {r_in,g_in,b_in};

		if (x_in==0 || x_in==1) begin
			{r_out,g_out,b_out} <= 24'b0;
		end
		else begin
			r_out <= ( r_in + prev_pixel[0][23:16] + prev_pixel[1][23:16] ) / 3;
			g_out <= ( g_in + prev_pixel[0][15:8] + prev_pixel[1][15:8] ) / 3;
			b_out <= ( b_in + prev_pixel[0][7:0] + prev_pixel[1][7:0] ) / 3;
		end
	end

end

endmodule
