module conv_filter_tb(
);


// inputs
logic clk;
logic rst;
logic valid;
reg[7:0] r_in, g_in, b_in;
reg [10:0] x_in, y_in;

// outputs
reg[7:0] r_out, g_out, b_out;

initial begin
    clk = 0;
    valid = 1;
    r_in = 0;
    g_in = 0;
    b_in = 0;

    x_in=0;
    y_in=0;


    // Number of image run-throughs
    repeat (5) begin

        repeat (1) begin

            repeat (50) begin

                clk = !clk;
                r_in = $urandom_range(0,255);
                g_in = $urandom_range(0,255);
                b_in = $urandom_range(0,255);
                #1;
                $display("      r_in= %d, g_in= %d, b_in= %d,  time=%t", r_in, g_in, b_in, $time);
                $display("avrg. r_out=%d, g_out=%d, b_out=%d", r_out, g_out, b_out);

                x_in=x_in+1;
            end
            y_in=y_in+1;
            x_in=0;

        end

        y_in=0;

        $display("======= NEW IMAGE FRAME =======");
    end

    $display("Finished. Total time = %t", $time);
    $finish;
end


CONV_FILTER m(
    .clk(clk),
    .rst(rst),
    .valid(valid),
    .r_in(r_in),
    .g_in(g_in),
    .b_in(b_in),
    .x_in(x_in),
    .y_in(y_in),
    .r_out(r_out),
    .g_out(g_out),
    .b_out(b_out)
);

endmodule
