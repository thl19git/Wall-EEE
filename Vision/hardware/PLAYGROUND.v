module playground(
);

reg [23:0] frame_history [480-1:0][640-1:0][2:0];
reg [7:0] r,g,b;

reg x;

initial begin
    r=8'hff;
    g=8'h55;
    b=8'h00;
    x=0;

    frame_history[0][0][0] = {r,g,b};

    repeat (30) begin

        r = $urandom_range(0,255);
        g = $urandom_range(0,255);
        b = $urandom_range(0,255);

        #1

        frame_history[x][0][2] <= frame_history[x][0][1];
        frame_history[x][0][1] <= frame_history[x][0][0];
        frame_history[x][0][0] <= {r,g,b};

        $display("R %b", frame_history[0][0][0]);
        $display("G %b", frame_history[0][0][1]);
        $display("B %b", frame_history[0][0][2]);
        $display("-------------");

    end

end

endmodule
