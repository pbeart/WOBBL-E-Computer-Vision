

#include <stdio.h>
#include "I2C_core.h"
#include "terasic_includes.h"
#include "mipi_camera_config.h"
#include "mipi_bridge_config.h"

#include "auto_focus.h"

#include <fcntl.h>
#include <unistd.h>

//EEE_IMGPROC defines
#define EEE_IMGPROC_MSG_START ('R'<<16 | 'B'<<8 | 'B')

//offsets
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG 1
#define EEE_IMGPROC_ID 2
#define EEE_IMGPROC_BBCOL 3
#define EEE_IMGPROC_THRESH 4

//#define EXPOSURE_INIT 0x002000
#define EXPOSURE_INIT 0x0
#define EXPOSURE_STEP 0x100
#define EXPOSURE_MAX 9000
//#define GAIN_INIT 0x080
#define GAIN_MAX 2048
#define GAIN_INIT 320
#define GAIN_STEP 0x040
#define threshold_step 0x5
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
	printf("\n{\"info\": \"PHY_status=%xh, CSI_status=%xh, MDLSynErr=%xh, FrmErrCnt=%xh, MDLErrCnt=%xh\"}\n", PHY_status, SCI_status, MDLSynErr,FrmErrCnt, MDLErrCnt);
	if (PHY_status || SCI_status || MDLSynErr || FrmErrCnt || MDLErrCnt) {
		printf("\n{\"error\": \"One or more key MIPI status registers was non-zero (error!), check log above for more information.\"}\n");
	}
}

void mipi_show_error_info_more(void){
	int error_yet = 0;
	int status;
	printf("\n{\"info\": \"MIPI Error Status: \"");

	status = MipiBridgeRegRead(0x0080);
    printf("FrmErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x0082);
	printf("CRCErrCnt = %d, ",status);
	error_yet += status;

	status = MipiBridgeRegRead(0x0084);
    printf("CorErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x0086);
    printf("HdrErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x0088);
    printf("EIDErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x008A);
    printf("CtlErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x008C);
    printf("SoTErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x008E);
    printf("SynErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x0090);
    printf("MDLErrCnt = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x00F8);
    printf("FIFOSTATUS = %d, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x006A);
    printf("DataType = 0x%04x, ",status);
    error_yet += status;

    status = MipiBridgeRegRead(0x006E);
    printf("CSIPktLen = %d, ",status);
    error_yet += status;

    printf("\"}\n");

    if (error_yet != 0) printf("\n{\"error\": \"One or more detailed MIPI status registers was non-zero (error!), check log above for more information.\"}\n");

}



bool MIPI_Init(void){
	bool bSuccess;


	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_MIPI_BASE, 50*1000*1000,400*1000); //I2C: 400K
	if (!bSuccess)
		printf("\n{\"error:\": \"failed to init MIPI- Bridge i2c\"}\n");

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

int key0_debounce = 0;
int message_index = 0;

int target_location_x = 0;
int target_location_y = 0;

int target_strength = 0;

int get_target_colour_by_index(int index) {
	switch(index) {
	   case 0:
		   return 0xFF7F00;
	   case 1:
		   return 0xffff00;
	   case 2:
		   return 0x00ff00;
	   case 3:
		   return 0x0000ff;
	   case 4:
		   return 0xff00ff;
	   }
	return 0xffffff;
}

int main()
{

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

  //printf("DE10-LITE D8M VGA Demo\n");
  //printf("Imperial College EEE2 Project version\n");
  printf("\n{\"info\": \"Hello!\"}\n");
  printf("\n{\"status\": \"earlystartup\"}\n");
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0x00);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0x00);

  usleep(2000);
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0xFF);
  usleep(2000);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0xFF);

  //printf("Image Processor ID: %x\n",IORD(0x42000,EEE_IMGPROC_ID));
  printf("\n{\"info\": \"Image Processor ID: %x\"}\n", IORD(0x42000,EEE_IMGPROC_ID));
  //printf("Image Processor ID: %x\n",IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_ID)); //Don't know why this doesn't work - definition is in system.h in BSP


  usleep(2000);
  printf("\n{\"status\": \"camera_init\"}\n");

  // MIPI Init
   if (!MIPI_Init()){
	  //printf("MIPI_Init Init failed!\r\n");
	  printf("\n{\"error\": \"MIPI_Init Init failed!\"}\n");
  }else{
	  //printf("MIPI_Init Init successfully!\r\n");
	  printf("\n{\"info\": \"MIPI_Init Init successfully!\"}\n");
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
    	alt_u16 threshold = 40000;

        OV8865SetExposure(exposureTime);
        OV8865SetGain(gain);
        Focus_Init();
        printf("\n{\"status\": \"uart_init\"}\n");
        FILE* ser = fopen("/dev/uart_0", "rb+");
        if(ser){
        	printf("\n{\"info\": \"Opened UART\"}\n");
        } else {
        	printf("\n{\"error\": \"Failed to open UART\"}\n");
        	while (1);
        }
        printf("\n{\"status\": \"running\"}\n");
  while(1){

       // touch KEY0 to trigger Auto focus
	   if((IORD(KEY_BASE,0)&0x03) == 0x02){

    	   current_focus = Focus_Window(320,240);
       }

#if 0
	   // touch KEY1 to ZOOM
	         if((IORD(KEY_BASE,0)&0x03) == 0x01){
	      	   if(bin_level == 3 )bin_level = 1;
	      	   else bin_level ++;
	      	   //printf("set bin level to %d\n",bin_level);
	      	   MIPI_BIN_LEVEL(bin_level);
	      	 	usleep(500000);

	         }

#endif

	#if 0
       if((IORD(KEY_BASE,0)&0x0F) == 0x0E){

    	   current_focus = Focus_Window(320,240);
       }

       // touch KEY1 to trigger Manual focus  - step
       if((IORD(KEY_BASE,0)&0x0F) == 0x0D){

    	   if(current_focus > manual_focus_step) current_focus -= manual_focus_step;
    	   else current_focus = 0;
    	   OV8865_FOCUS_Move_to(current_focus);

       }

       // touch KEY2 to trigger Manual focus  + step
       if((IORD(KEY_BASE,0)&0x0F) == 0x0B){
    	   current_focus += manual_focus_step;
    	   if(current_focus >1023) current_focus = 1023;
    	   OV8865_FOCUS_Move_to(current_focus);
       }

       // touch KEY3 to ZOOM
       if((IORD(KEY_BASE,0)&0x0F) == 0x07){
    	   if(bin_level == 3 )bin_level = 1;
    	   else bin_level ++;
    	   //printf("set bin level to %d\n",bin_level);
    	   MIPI_BIN_LEVEL(bin_level);
    	 	usleep(500000);

       }
	#endif


       //Read messages from the image processor and print them on the terminal
       while ((IORD(0x42000,EEE_IMGPROC_STATUS)>>8) & 0xff) { 	//Find out if there are words to read
           int word = IORD(0x42000,EEE_IMGPROC_MSG); 			//Get next word from message buffer
    	   if (fwrite(&word, 4, 1, ser) != 1)
    		   //printf("Error writing to UART");
    		   printf("\n{\"error\": \"UART write error\"}\n");
           if (word == EEE_IMGPROC_MSG_START) {				//Newline on message identifier
    		   //printf("-----------------\n");
           	   	   message_index = 0;
           }

           if (message_index == 1) {
        	   // these 32 bits look like: {5'b0, matchiest_pixel_x, 5'b0, matchiest_pixel_y}
        	   target_location_x = (word & 0xffff0000)>>16;
        	   target_location_y = word & 0xffff;
        	   //printf("    Location X,Y: (%i, %i)\n", , );
        	   //printf("%08x ",word);
           } else if (message_index == 2) {
        	   //printf("    Magic: %08x\n", word);
        	   //target_strength = word & 0x7ffff;
        	   target_strength = word & 0xffffff;
           }
           message_index++;
       }

       // If key1 pressed, switch target colours
       if((IORD(KEY_BASE,0)&0x03) == 0x01){
    	   if (key0_debounce) {
			   boundingBoxColour++;
			   boundingBoxColour = boundingBoxColour % 5;
			   IOWR(0x42000, EEE_IMGPROC_BBCOL, get_target_colour_by_index(boundingBoxColour));
    	   }
		   key0_debounce = 0;
		 } else {
			 key0_debounce = 1;
		 }
       /*
       boundingBoxColour = ((boundingBoxColour + 1) & 0xff);
       IOWR(0x42000, EEE_IMGPROC_BBCOL, (boundingBoxColour << 8) | (0xff - boundingBoxColour));
		*/


       //Process input commands
       int in = getchar();
       switch (in) {
       	   case 'e': {
       		   exposureTime += EXPOSURE_STEP;
       		   if (exposureTime > EXPOSURE_MAX) exposureTime = EXPOSURE_MAX;
       		   OV8865SetExposure(exposureTime);
       		   //printf("\nExposure = %x ", exposureTime);
       	   	   break;}
       	   case 'd': {
       		   exposureTime -= EXPOSURE_STEP;
       		   if (exposureTime < 0) exposureTime = 0;
       		   OV8865SetExposure(exposureTime);
       		   //printf("\nExposure = %x ", exposureTime);
       	   	   break;}
       	   case 't': {
       		   gain += GAIN_STEP;
       		   if (gain > GAIN_MAX) gain = GAIN_MAX;
       		   OV8865SetGain(gain);
       		   //printf("\nGain = %x ", gain);
       	   	   break;}
       	   case 'g': {
       		   gain -= GAIN_STEP;
       		   if (gain < 0) gain = 0;
       		   OV8865SetGain(gain);
       		   //printf("\nGain = %x ", gain);
       	   	   break;}
       	   case 'r': {
        	   current_focus += manual_focus_step;
        	   if(current_focus >1023) current_focus = 1023;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   //printf("\nFocus = %x ",current_focus);
       	   	   break;}
       	   case 'f': {
        	   if(current_focus > manual_focus_step) current_focus -= manual_focus_step;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   //printf("\nFocus = %x ",current_focus);
       	   	   break;}
       	   case 'y': {
       		   threshold *= 1.05;
			   IOWR(0x42000, EEE_IMGPROC_THRESH, threshold);
			   //printf("\nFocus = %x ",current_focus);
			   break;}
		   case 'h': {
			   threshold /= 1.05;
			   threshold++; // don't let threshold fall to 0, because it wouldn't be increasable by multiplying
			   IOWR(0x42000, EEE_IMGPROC_THRESH, threshold);
			   //printf("\nFocus = %x ",current_focus);
			   break;}
       }
       printf("\n{");
       printf("\"current_search_colour\": %d,", get_target_colour_by_index(boundingBoxColour));
       printf("\"target_location\": [%d, %d],", target_location_x, target_location_y);
       printf("\"cam_gain\": %d,", gain);
       printf("\"cam_exposure\": %d,", exposureTime);
       printf("\"cam_focus\": %d,", current_focus);
       printf("\"sens_threshold\": %d,", threshold);
       //printf("\"target_strength\": %d,", target_strength);
       printf("\"target_strength\": %06x,", target_strength);
       printf("}\n");


	   //Main loop delay
	   usleep(10000);

   };
  return 0;
}
