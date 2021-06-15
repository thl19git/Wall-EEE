

#include <stdio.h>
#include "I2C_core.h"
#include "terasic_includes.h"
#include "mipi_camera_config.h"
#include "mipi_bridge_config.h"

#include "auto_focus.h"

#include <fcntl.h>
#include <unistd.h>

// For the ball size estimation
#include "math.h"
#include <stdlib.h>

//EEE_IMGPROC defines
#define R_BOX_MSG_ID = 0xFFFF0001;
#define Y_BOX_MSG_ID = 0xFFFF0002;
#define G_BOX_MSG_ID = 0xFFFF0003;
#define B_BOX_MSG_ID = 0xFFFF0004;
#define P_BOX_MSG_ID = 0xFFFF0005;


//offsets
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG 1
#define EEE_IMGPROC_ID 2
#define EEE_IMGPROC_BBCOL 3

#define EXPOSURE_INIT 0x002000
#define EXPOSURE_STEP 0x100
#define GAIN_INIT 0x300
#define GAIN_STEP 0x300
#define DEFAULT_LEVEL 3

#define MIPI_REG_PHYClkCtl		0x0056
#define MIPI_REG_PHYData0Ctl	0x0058
#define MIPI_REG_PHYData1Ctl	0x005A
#define MIPI_REG_PHYData2Ctl	0x005C
#define MIPI_REG_PHYData3Ctl	0x005E
#define MIPI_REG_PHYTimDly		0x0060
#define MIPI_REG_PHYSta			0x0062
#define MIPI_REG_CSIStatus		0x0064
#define MIPI_REG_CSIErrEn		0x0066
#define MIPI_REG_MDLSynErr		0x0068
#define MIPI_REG_FrmErrCnt		0x0080
#define MIPI_REG_MDLErrCnt		0x0090

void mipi_clear_error(void){
	MipiBridgeRegWrite(MIPI_REG_CSIStatus,0x01FF); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLSynErr,0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_FrmErrCnt,0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLErrCnt, 0x0000); // clear error

  	MipiBridgeRegWrite(0x0082,0x00);
  	MipiBridgeRegWrite(0x0084,0x00);
  	MipiBridgeRegWrite(0x0086,0x00);
  	MipiBridgeRegWrite(0x0088,0x00);
  	MipiBridgeRegWrite(0x008A,0x00);
  	MipiBridgeRegWrite(0x008C,0x00);
  	MipiBridgeRegWrite(0x008E,0x00);
  	MipiBridgeRegWrite(0x0090,0x00);
}

void mipi_show_error_info(void){

	alt_u16 PHY_status, SCI_status, MDLSynErr, FrmErrCnt, MDLErrCnt;

	PHY_status = MipiBridgeRegRead(MIPI_REG_PHYSta);
	SCI_status = MipiBridgeRegRead(MIPI_REG_CSIStatus);
	MDLSynErr = MipiBridgeRegRead(MIPI_REG_MDLSynErr);
	FrmErrCnt = MipiBridgeRegRead(MIPI_REG_FrmErrCnt);
	MDLErrCnt = MipiBridgeRegRead(MIPI_REG_MDLErrCnt);
	printf("PHY_status=%xh, CSI_status=%xh, MDLSynErr=%xh, FrmErrCnt=%xh, MDLErrCnt=%xh\r\n", PHY_status, SCI_status, MDLSynErr,FrmErrCnt, MDLErrCnt);
}

void mipi_show_error_info_more(void){
    printf("FrmErrCnt = %d\n",MipiBridgeRegRead(0x0080));
    printf("CRCErrCnt = %d\n",MipiBridgeRegRead(0x0082));
    printf("CorErrCnt = %d\n",MipiBridgeRegRead(0x0084));
    printf("HdrErrCnt = %d\n",MipiBridgeRegRead(0x0086));
    printf("EIDErrCnt = %d\n",MipiBridgeRegRead(0x0088));
    printf("CtlErrCnt = %d\n",MipiBridgeRegRead(0x008A));
    printf("SoTErrCnt = %d\n",MipiBridgeRegRead(0x008C));
    printf("SynErrCnt = %d\n",MipiBridgeRegRead(0x008E));
    printf("MDLErrCnt = %d\n",MipiBridgeRegRead(0x0090));
    printf("FIFOSTATUS = %d\n",MipiBridgeRegRead(0x00F8));
    printf("DataType = 0x%04x\n",MipiBridgeRegRead(0x006A));
    printf("CSIPktLen = %d\n",MipiBridgeRegRead(0x006E));
}



bool MIPI_Init(void){
	bool bSuccess;


	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_MIPI_BASE, 50*1000*1000,400*1000); //I2C: 400K
	if (!bSuccess)
		printf("failed to init MIPI- Bridge i2c\r\n");

    usleep(50*1000);
    MipiBridgeInit();

    usleep(500*1000);

//	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_CAMERA_BASE, 50*1000*1000,400*1000); //I2C: 400K
//	if (!bSuccess)
//		printf("failed to init MIPI- Camera i2c\r\n");

    MipiCameraInit();
    MIPI_BIN_LEVEL(DEFAULT_LEVEL);
//    OV8865_FOCUS_Move_to(340);

//    oc_i2c_uninit(I2C_OPENCORES_CAMERA_BASE);  // Release I2C bus , due to two I2C master shared!


 	usleep(1000);


//    oc_i2c_uninit(I2C_OPENCORES_MIPI_BASE);

	return bSuccess;
}


// Helper functions to convert the bounding box to a relative position the ball
alt_32 get_x(int x1, int x2, int y1, int y2){
    int ball_size = abs(x2-x1);                                     //ball_size in pixels
    float distance = 20 * sqrt(pow(1375/ball_size, 2) - 1);         //from the matlab formula calculated ealier
    float angle =((x1 + (ball_size/2) - 320)/640)*0.261799;         //field of view is 15 degrees (0.261799 radians), takes the middle position of the ball on the frame to calulate angle
    if(angle >= 0){
        return (alt_32)(distance*sin(angle));
    }else{
        return (alt_32)(distance*sin(angle)*-1);
    }
}

alt_32 get_y(int x1, int x2, int y1, int y2){
    int ball_size = abs(x2-x1);                                     //ball_size in pixels
    float distance = 20 * sqrt(pow(1375/ball_size, 2) - 1);         //from the matlab formula calculated ealier
    float angle =((x1 + (ball_size/2) - 320)/640)*0.261799;         //field of view is 15 degrees (0.261799 radians), takes the middle position of the ball on the frame to calulate angle
    if(angle >= 0){
        return (alt_32)(distance*cos(angle));
    }else{
        return (alt_32)(distance*cos(angle)*-1);
    };
}


void update_ball_coords(int color, int *ball_coords[5][4]) {
	int msg1 = IORD(0x42000,EEE_IMGPROC_MSG);				// Read the first message after the identifier
	int msg2 = IORD(0x42000,EEE_IMGPROC_MSG);				// Read the second message after the identifier
	printf("\n Ball number: %d \n", color);
	printf("MSG1:");
	printf("%08x",msg1);
	printf("\n");
	printf("MSG2:");
	printf("%08x",msg2);
	printf("\n");
	*ball_coords[0][0] = ((msg1 & 0xFFFF0000) >> 16);		// Bitmask and shift to retrieve the relevant data ...
	*ball_coords[0][1] = (msg1 & 0x0000FFFF);
	*ball_coords[0][2] = ((msg2 & 0xFFFF0000) >> 16);
	*ball_coords[0][3] = (msg2 & 0x0000FFFF);
};



int main()
{

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

  printf("DE10-LITE D8M VGA Demo\n");
  printf("Imperial College EEE2 Project version\n");
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0x00);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0x00);

  usleep(2000);
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0xFF);
  usleep(2000);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0xFF);

  printf("Image Processor ID: %x\n",IORD(0x42000,EEE_IMGPROC_ID));
  //printf("Image Processor ID: %x\n",IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_ID)); //Don't know why this doesn't work - definition is in system.h in BSP


  usleep(2000);


  // MIPI Init
   if (!MIPI_Init()){
	  printf("MIPI_Init Init failed!\r\n");
  }else{
	  printf("MIPI_Init Init successfully!\r\n");
  }

//   while(1){
 	    mipi_clear_error();
	 	usleep(50*1000);
 	    mipi_clear_error();
	 	usleep(1000*1000);
	    mipi_show_error_info();
//	    mipi_show_error_info_more();
	    printf("\n");
//   }


#if 0  // focus sweep
	    printf("\nFocus sweep\n");
 	 	alt_u16 ii= 350;
 	    alt_u8  dir = 0;
 	 	while(1){
 	 		if(ii< 50) dir = 1;
 	 		else if (ii> 1000) dir =0;

 	 		if(dir) ii += 20;
 	 		else    ii -= 20;

 	    	printf("%d\n",ii);
 	     OV8865_FOCUS_Move_to(ii);
 	     usleep(50*1000);
 	    }
#endif


    //////////////////////////////////////////////////////////
        alt_u16 bin_level = DEFAULT_LEVEL;
        alt_u8  manual_focus_step = 10;
        alt_u16  current_focus = 300;
    	int boundingBoxColour = 0;
    	alt_u32 exposureTime = EXPOSURE_INIT;
    	alt_u16 gain = GAIN_INIT;

        OV8865SetExposure(exposureTime);
        OV8865SetGain(gain);
        Focus_Init();

        FILE* ser = fopen("/dev/uart_0", "rb+");
        if(ser){
        	printf("Opened UART\n");
        } else {
        	printf("Failed to open UART\n");
        	//while (1);
        }

  while(1){

       // touch KEY0 to trigger Auto focus
	   if((IORD(KEY_BASE,0)&0x03) == 0x02){
    	   current_focus = Focus_Window(320,240);
       }
	   // touch KEY1 to ZOOM
	         if((IORD(KEY_BASE,0)&0x03) == 0x01){
	      	   if(bin_level == 3 )bin_level = 1;
	      	   else bin_level ++;
	      	   printf("set bin level to %d\n",bin_level);
	      	   MIPI_BIN_LEVEL(bin_level);
	      	 	usleep(500000);
		}

	    // This 2d arrays store the bounding box data (left, right, top, bottom) for all ball colors RYGBP
		int ball_coords[5][4];

       //Read messages from the image processor and stores the latest available position in variable
       while ((IORD(0x42000,EEE_IMGPROC_STATUS)>>8) & 0xff) { 	// Find out if there are words to read

           int word = IORD(0x42000,EEE_IMGPROC_MSG); 			// Get next word from message buffer
		   switch(word) {
			   case 0:
				    update_ball_coords(0, &ball_coords);
				    break;
			   case 1:
			   		update_ball_coords(1, &ball_coords);
				    break;
			   case 2:
			   		update_ball_coords(2, &ball_coords);
					break;
			   case  3:
			   		update_ball_coords(3, &ball_coords);
					break;
			   case  4:
			   		update_ball_coords(4, &ball_coords);
					break;
		    }
    	   // printf("%08x ",word);
       }

	   // First check which balls were detected
	   bool balls_found[5]={0,0,0,0,0}; // RYGBP
	   for(int i=0; i<5; i++) {
		   if(ball_coords[i][0]!=0 && ball_coords[i][1]!=640 && ball_coords[i][2]!=0 && ball_coords[i][3]!=320) {
			   balls_found[i]=1; // Ball is found
		   }
	   }

	   // After the onchip serial buffer is read, the bounding box is calculated
	   alt_u32 uart_esp_packets[6];

	   // Construct the first packet
	   // Packet 0 has got format 0xFFFRYGBP (where RYGBP are bool flags indicating existence of the ball)
	   uart_esp_packets[0] = ( 0xFFF00000 | ((alt_u32)(balls_found[0]) << 4) | ((alt_u32)(balls_found[1]) << 3) | ((alt_u32)(balls_found[2]) << 2) | ((alt_u32)(balls_found[3]) << 1) | (alt_u32)(balls_found[3]) );
	   // printf("First packet: %08x ",uart_esp_packets[i]);

	   // Construct the following 5 ball position packets
	   for(int i=0; i<5; i++) {
		   alt_32 relative_distance_x = get_x(ball_coords[i][0],ball_coords[i][1],ball_coords[i][2],ball_coords[i][3]);
		   alt_32 relative_distance_y = get_y(ball_coords[i][0],ball_coords[i][1],ball_coords[i][2],ball_coords[i][3]);
		   uart_esp_packets[i+1] = (relative_distance_x<<16) | relative_distance_y;
	   }

	   // Print out all the position packets to the ESP8266 via UART
	   for(int i=0; i<6; i++) {
		   // printf("%08x ",uart_esp_packets[i]);
		   // printf("\n");
		   if (fwrite(&uart_esp_packets[i], 4, 1, ser) != 1)
			   printf("Error writing to UART");
	   }


       //Update the bounding box colour
       boundingBoxColour = ((boundingBoxColour + 1) & 0xff);
       IOWR(0x42000, EEE_IMGPROC_BBCOL, (boundingBoxColour << 8) | (0xff - boundingBoxColour));

       //Process input commands
       int in = getchar();
       switch (in) {
       	   case 'e': {
       		   exposureTime += EXPOSURE_STEP;
       		   OV8865SetExposure(exposureTime);
       		   printf("\nExposure = %x ", exposureTime);
       	   	   break;}
       	   case 'd': {
       		   exposureTime -= EXPOSURE_STEP;
       		   OV8865SetExposure(exposureTime);
       		   printf("\nExposure = %x ", exposureTime);
       	   	   break;}
       	   case 't': {
       		   gain += GAIN_STEP;
       		   OV8865SetGain(gain);
       		   printf("\nGain = %x ", gain);
       	   	   break;}
       	   case 'g': {
       		   gain -= GAIN_STEP;
       		   OV8865SetGain(gain);
       		   printf("\nGain = %x ", gain);
       	   	   break;}
       	   case 'r': {
        	   current_focus += manual_focus_step;
        	   if(current_focus >1023) current_focus = 1023;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   printf("\nFocus = %x ",current_focus);
       	   	   break;}
       	   case 'f': {
        	   if(current_focus > manual_focus_step) current_focus -= manual_focus_step;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   printf("\nFocus = %x ",current_focus);
       	   	   break;}
       }


	   //Main loop delay
	   usleep(10000);

   };
  return 0;
}
