module CONV_FILTER(
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

    The x and y pixel coordinates might by offset by +1 each, due to the nature of when the pixel information is available.
    The x and y in the parent therefore are are offset by +1, which should be accounted for in the parent module EEE_IMGPROC.

*/

// reg [23:0] previous_rows [640-1:0][2:0];
//
// always @(posedge clk) begin
//
// 	if(valid) begin
//
// 	    previous_rows[x_in][2] <= previous_rows[x_in][1];
// 	    previous_rows[x_in][1] <= previous_rows[x_in][0];
// 	    previous_rows[x_in][0] <= {r_in,g_in,b_in};
//
// 	    // Make all 2 edge pixels black
// 	    if (x_in<=1 || x_in>=638 || y_in<=1 || y_in>=478) begin
// 	        {r_out,g_out,b_out} <= 24'b0;
// 	    end
// 	    // Take a 'spatial' average of the image, by using a 3x3 averaging filter
// 	    else begin
//
// 	        r_out <= (previous_rows[x_in-2][0][7:0] + previous_rows[x_in-1][0][7:0] + previous_rows[x_in-2][1][7:0] + previous_rows[x_in-1][1][7:0] + previous_rows[x_in][1][7:0] + previous_rows[x_in-2][2][7:0] + previous_rows[x_in-1][2][7:0] + previous_rows[x_in][2][7:0] + r_in ) / 9;
// 	        g_out <= (previous_rows[x_in-2][0][15:8] + previous_rows[x_in-1][0][15:8] + previous_rows[x_in-2][1][15:8] + previous_rows[x_in-1][1][15:8] + previous_rows[x_in][1][15:8] + previous_rows[x_in-2][2][15:8] + previous_rows[x_in-1][2][15:8] + previous_rows[x_in][2][15:8] + g_in ) / 9;
// 	        b_out <= (previous_rows[x_in-2][0][23:16] + previous_rows[x_in-1][0][23:16] + previous_rows[x_in-2][1][23:16] + previous_rows[x_in-1][1][23:16] + previous_rows[x_in][1][23:16] + previous_rows[x_in-2][2][23:16] + previous_rows[x_in-1][2][23:16] + previous_rows[x_in][2][23:16] + r_in ) / 9;
// 	    end
//
// 	end
//
// end

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

		if (x_in==0) begin
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
