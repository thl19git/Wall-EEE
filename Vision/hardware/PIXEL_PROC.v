module PIXEL_PROC(
    input               clk,
    input               rst,
    input       [23:0]  pixel_in,               // The raw concatenated 8-bit RGB channels --> IN
    output reg  [2:0]   pixel_classification    // 0:unclassified | 1:red ball | 2:yellow ball | 3:green ball | 4:blue ball | 5:pink ball
);

/*

 The HSV color ranges can be best tuned by capturing a raw image
 with the classication targets (ping pong balls) and then tuning the ranges
 through Matlab's Color Thresholder in the Image Processing Toolbox


The HSV hue spectrum:
    |0=========================180=========================360|
    | red .... yellow .... green .... blue .... pink .... red |
    |=========================================================|

*/

////////////////////////////

// Separate wires for the R-G-B colour channels
wire [7:0]   red, green, blue;
assign {red,green,blue} = pixel_in;

wire [8:0] hsv_h;
wire [7:0] hsv_s, hsv_v;

always @(*) begin
    // Red ball
    if((hsv_h < 27 | hsv_h > 359) &  hsv_s>68 & hsv_v>24) begin
        pixel_classification <= 3'h1;
    end
    // Yellow ball
    else if((hsv_h > 68 & hsv_h < 79) & hsv_s>57 & hsv_v>65) begin
        pixel_classification <= 3'h2;
    end
    // Green ball
    else if((hsv_h > 137 & hsv_h < 170) & hsv_s>54 & hsv_v>23) begin
        pixel_classification <= 3'h3;
    end
    // Blue ball
    else if((hsv_h > 187 & hsv_h < 241) & hsv_s>35 & hsv_v>1) begin
        pixel_classification <= 3'h4;
    end
    // Pink ball
    else if((hsv_h > 0 & hsv_h < 31) & hsv_s>30 & hsv_v>58) begin
        pixel_classification <= 3'h5;
    end
    // Colour not a ping pong ball
    else begin
        pixel_classification <= 3'h0;
    end

end


// Hardware to convert rgb to hsv color-space
// This hardware block outputs with 2 clock cycles delay
RGB_2_HSV rgb_2_hsv(
    .clk(clk),
    .rst(rst),
    .rgb_r(red),
    .rgb_g(green),
    .rgb_b(blue),
    .hsv_h(hsv_h),
    .hsv_s(hsv_s),
    .hsv_v(hsv_v)
);

endmodule
