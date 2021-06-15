// iverilog -Wall -g 2012 -s rgb2hsv_tb -o rgb_hsv_testbench RGB_2_HSV.v RGB_2_HSV_TB.v

module rgb2hsv_tb(
);

    // RGB 2 HSV inputs
    logic clk;
    logic rst;
    reg[7:0] r_in, g_in, b_in;

    // RGB 2 HSV outputs
    reg[8:0] hsv_h;
    reg[7:0] hsv_s;
    reg[7:0] hsv_v;

    initial begin
        clk = 0;
        r_in = 0;
        g_in = 0;
        b_in = 0;

         repeat (100) begin

            clk = !clk;
            #1;

            $display("r=%d, g=%d, b=%d,  time=%t", r_in, g_in, b_in, $time);
            $display("      --> h=%d, s=%d, v=%d", hsv_h, hsv_s, hsv_v);

            // r_in = $urandom_range(0,255);
            // g_in = $urandom_range(0,255);
            // b_in = $urandom_range(0,255);

            r_in = 255;
            g_in = 1;
            b_in = 1;

            clk = !clk;
        end

        $display("Finished. Total time = %t", $time);
        $finish;
    end



    RGB_2_HSV m(
        .clk(clk),
        .rst(rst),
        .rgb_r(r_in),
        .rgb_g(g_in),
        .rgb_b(b_in),
        .hsv_h(hsv_h),
        .hsv_s(hsv_s),
        .hsv_v(hsv_v)
    );

endmodule
