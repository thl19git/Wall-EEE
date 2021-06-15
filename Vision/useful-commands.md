# Useful commands for this project

### Navigate into the Quartus Nios CLI Folder

`cd /cygdrive/c/Users/Kunststoffe/Desktop/EEE2Rover-master/DE10_LITE_D8M_VIP_16/software/D8M_Camera_Test/`


### Start the Nios2 Command Shell

`& '.\Nios II Command Shell.bat'`


### Download the generated software onto the Nios2 Processor that is running

`nios2-download D8M_Camera_Test.elf -c 1 -g`


### Compiling the test-bench commands

#### PIXEL_PROC
`iverilog -Wall -g 2012 -s pixel_proc_tb -o pixel_proc_tb.out RGB_2_HSV.v PIXEL_PROC.v PIXEL_PROC_TB.v`

#### EEE_IMGPROC
`iverilog -Wall -g 2012 -s eee_imgproc_tb -o eee_imgproc_tb.out AVER_FILTER.v PIXEL_PROC.v RGB_2_HSV.v STREAM_REG.v MSG_FIFO.v EEE_IMGPROC.v EEE_IMGPROC_TB.v`

#### TEMPORAL_FILTER
`iverilog -Wall -g 2012 -s temporal_filter_tb -o temporal_filter_tb.out TEMPORAL_FILTER.v TEMPORAL_FILTER_TB.v`

#### CONVOLUTIONAL_FILTER
`iverilog -Wall -g 2012 -s conv_filter_tb -o conv_filter_tb.out CONV_FILTER.v CONV_FILTER_TB.v`

#### PLAYGROUND
`iverilog -Wall -g 2012 -s playground -o playground.out PLAYGROUND.v`
