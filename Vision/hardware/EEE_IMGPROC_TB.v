module eee_imgproc_tb();

// Global Inputs;
logic clk, reset_n;

// mm slave
logic s_chipselect, s_read, s_write;
reg	[31:0] s_readdata, s_writedata;
reg [2:0] s_address;

// streaming sink
logic sink_valid, sink_ready, sink_sop, sink_eop;
reg [23:0] sink_data;

// streaming source
logic source_valid, source_ready, source_sop, source_eop;
reg [23:0] source_data;

// conduit export
logic mode;

initial begin

    // Set the initial block inputs
    clk             = 0;
    reset_n         = 1;
    s_chipselect    = 1;
    s_read          = 1;
    s_write         = 1;
    s_writedata     = 1;
    s_address       = 1;

    sink_data       = 1;
    sink_valid      = 1;
    sink_sop        = 1;
    sink_eop        = 0;

    source_ready    = 1;
    mode            = 1;

    #1

    reset_n = 0;

    #1

    reset_n=1;
    repeat (100) begin

       clk = !clk;

       #1;

       sink_data = $urandom_range(24'h000000, 24'hffffff);
       // $display("Out: %h", source_data);
       clk = !clk;

       #1

       sink_sop = 0;

   end

   sink_eop = 1;

   $display("Finished. Total time = %t", $time);
   $finish;


end

EEE_IMGPROC m(
    .clk(clk),
    .reset_n(reset_n),
    .s_chipselect(s_chipselect),
    .s_read(s_read),
    .s_write(s_write),
    .s_readdata(s_readdata),
    .s_writedata(s_writedata),
    .s_address(s_address),
    .sink_data(sink_data),
    .sink_valid(sink_valid),
    .sink_ready(sink_ready),
    .sink_sop(sink_sop),
    .sink_eop(sink_eop),
    .source_data(source_data),
    .source_valid(source_valid),
    .source_ready(source_ready),
    .source_sop(source_sop),
    .source_eop(source_eop),
    .mode(mode)
);


endmodule

/*
    iverilog -Wall -g 2012 -s eee_imgproc_tb -o eee_imgproc_tb.out PIXEL_PROC.v RGB_2_HSV.v STREAM_REG.v EEE_IMGPROC_TB.v EEE_IMGPROC.v
*/
