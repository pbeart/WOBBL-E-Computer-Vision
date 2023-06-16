#include "logging.h"
#include <stdio.h>
#include "I2C_core.h"
#include "terasic_includes.h"
#include "mipi_camera_config.h"
#include "mipi_bridge_config.h"

#include "auto_focus.h"

#include <fcntl.h>
#include <unistd.h>
#include <time.h>

//EEE_IMGPROC defines
#define EEE_IMGPROC_MSG_START ('R'<<16 | 'B'<<8 | 'B')

//offsets
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG 1
#define EEE_IMGPROC_ID 2
#define EEE_IMGPROC_BBCOL 3
#define EEE_IMGPROC_THRESH 4
#define EEE_IMGPROC_PIXTHRESH 5

//#define EXPOSURE_INIT 0x002000
#define EXPOSURE_INIT 552//552
#define EXPOSURE_STEP 0xF0
#define EXPOSURE_MAX 9000
//#define GAIN_INIT 0x080
#define GAIN_MAX 2048
#define GAIN_INIT 640//640

#define GAIN_STEP 0x010
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

#define EEE_IMGPROC_BASE 0x41020//0x42000

#define OUTPUT_EVERY_CLOCK_TICKS 500

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
	printf("\r{\"info\": \"PHY_status=%xh, CSI_status=%xh, MDLSynErr=%xh, FrmErrCnt=%xh, MDLErrCnt=%xh\"}\r", PHY_status, SCI_status, MDLSynErr,FrmErrCnt, MDLErrCnt);
	if (PHY_status || SCI_status || MDLSynErr || FrmErrCnt || MDLErrCnt) {
		printf("\r{\"error\": \"One or more key MIPI status registers was non-zero (error!), check log above for more information.\"}\r");
	}
}

void mipi_show_error_info_more(void){

	int error_yet = 0;
	int status;
	printf("\r{\"info\": \"MIPI Error Status: \"");

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

    printf("\"}\r");

    if (error_yet != 0) printf("\r{\"error\": \"One or more detailed MIPI status registers was non-zero (error!), check log above for more information.\"}\r");

}



bool MIPI_Init(void){
	bool bSuccess;


	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_MIPI_BASE, 50*1000*1000,400*1000); //I2C: 400K
	if (!bSuccess)
		printf("\r{\"error:\": \"failed to init MIPI- Bridge i2c\"}\r");

    usleep(50*1000);
    MipiBridgeInit();

    usleep(500*1000);

//	bSuccess = oc_i2c_init_ex(I2C_OPENCORES_CAMERA_BASE, 50*1000*1000,400*1000); //I2C: 400K
//	if (!bSuccess)
//		printf("failed to init MIPI- Camera i2c\r\r");

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
	   case 0: // red target colour
		   return 0xFF1938;
	   case 1: // orange target colour
		   return 0xE09D00;
	   case 2: // blue target colour
		   return 0x0040EF;
	   }
	return 0xffffff;
}

// 0 = reading in key
// 1 = reading in value
int patson_sm_state = 0;

#define PATSON_SM_BUFFER_SIZE 32

char patson_sm_key_buffer[32];
int patson_sm_key_pointer = 0;

char patson_sm_val_buffer[32];
int patson_sm_val_pointer = 0;

int read_ser;

int cam_target_colour = 0xff0000;

void cam_set_target_colour(int col) {
	cam_target_colour = col;
	IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_BBCOL, cam_target_colour);
}


int main()
{

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

	printf("\r{\"status\": \"uart_init\"}\r");
	ser = fopen("/dev/uart_0", "wb+"); // rb+

	read_ser = open("/dev/uart_0", O_RDWR|O_NONBLOCK);
	if(ser){
		comm_log("\r{\"info\": \"Opened UART\"}\r");
	} else {
		comm_log("\r{\"error\": \"Failed to open UART\"}\r");
		while (1);
	}

  //printf("DE10-LITE D8M VGA Demo\r");
  //printf("Imperial College EEE2 Project version\r");
	comm_log("\r{\"info\": \"Hello!\"}\r");
	comm_log("\r{\"status\": \"earlystartup\"}\r");
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0x00);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0x00);

  usleep(2000);
  IOWR(MIPI_PWDN_N_BASE, 0x00, 0xFF);
  usleep(2000);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0xFF);

  //printf("Image Processor ID: %x\r",IORD(0x42000,EEE_IMGPROC_ID));
  int imageprocessorid = IORD(EEE_IMGPROC_BASE,EEE_IMGPROC_ID);
  comm_log("\r{\"info\": \"Image Processor ID: %x\"}\r", imageprocessorid);
  //printf("Image Processor ID: %x\r",IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_ID)); //Don't know why this doesn't work - definition is in system.h in BSP
  alt_u32 intended_id = 0x1234eee2;
  if (imageprocessorid != intended_id) {
	  comm_log("\r{\"error\": \"Image Processor ID does not match %x (was %x)\"}\r", intended_id, imageprocessorid);
  }

  usleep(2000);
  comm_log("\r{\"status\": \"camera_init\"}\r");

  // MIPI Init
   if (!MIPI_Init()){
	  //printf("MIPI_Init Init failed!\r\r");
	   comm_log("\r{\"error\": \"MIPI_Init Init failed!\"}\r");
  }else{
	  //printf("MIPI_Init Init successfully!\r\r");
	  comm_log("\r{\"info\": \"MIPI_Init Init successfully!\"}\r");
  }


//   while(1){
 	    mipi_clear_error();
	 	usleep(50*1000);
 	    mipi_clear_error();
	 	usleep(1000*1000);
	    mipi_show_error_info();
//	    mipi_show_error_info_more();
	    printf("\r");
//   }


#if 0  // focus sweep
	    printf("\rFocus sweep\r");
 	 	alt_u16 ii= 350;
 	    alt_u8  dir = 0;
 	 	while(1){
 	 		if(ii< 50) dir = 1;
 	 		else if (ii> 1000) dir =0;

 	 		if(dir) ii += 20;
 	 		else    ii -= 20;

 	    	printf("%d\r",ii);
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
    	alt_u16 threshold = 7000;
    	//threshold =

    	alt_u16 pix_threshold = 10;

        OV8865SetExposure(exposureTime);
        OV8865SetGain(gain);
        Focus_Init();

        IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_THRESH, threshold);
        IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_PIXTHRESH, pix_threshold);

        comm_log("\r{\"status\": \"running\"}\r");

        clock_t last_output = 0;
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
	      	   //printf("set bin level to %d\r",bin_level);
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
    	   //printf("set bin level to %d\r",bin_level);
    	   MIPI_BIN_LEVEL(bin_level);
    	 	usleep(500000);

       }
	#endif

       printf("test a");
       //Read messages from the image processor and print them on the terminal
       while ((IORD(EEE_IMGPROC_BASE,EEE_IMGPROC_STATUS)>>8) & 0xff) { 	//Find out if there are words to read
    	   printf("test b");
    	   int word = IORD(EEE_IMGPROC_BASE,EEE_IMGPROC_MSG); 			//Get next word from message buffer
    	   //word=69;
           //if (fwrite(&word, 4, 1, ser) != 1)
    		   //printf("Error writing to UART");
    		//   printf("\r{\"error\": \"UART write error\"}\r");
           if (word == EEE_IMGPROC_MSG_START) {				//Newline on message identifier
    		   //printf("-----------------\r");
           	   	   message_index = 0;
           }
           printf("test c");
           if (message_index == 1) {
        	   // these 32 bits look like: {5'b0, matchiest_pixel_x, 5'b0, matchiest_pixel_y}
        	   target_location_x = (word & 0xffff0000)>>16;
        	   target_location_y = word & 0xffff;
        	   //printf("    Location X,Y: (%i, %i)\r", , );
        	   //printf("%08x ",word);
           } else if (message_index == 2) {
        	   //printf("    Magic: %08x\r", word);
        	   //target_strength = word & 0x7ffff;
        	   target_strength = word & 0xffffff;
           }
           message_index++;
       }

       // If key1 pressed, switch target colours
       if((IORD(KEY_BASE,0)&0x03) == 0x01){
    	   if (key0_debounce) {
			   boundingBoxColour++;
			   boundingBoxColour = boundingBoxColour % 3;
			   //IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_BBCOL, get_target_colour_by_index(boundingBoxColour));
			   cam_set_target_colour(get_target_colour_by_index(boundingBoxColour));
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
       		   //printf("\rExposure = %x ", exposureTime);
       	   	   break;}
       	   case 'd': {
       		   exposureTime -= EXPOSURE_STEP;
       		   if (exposureTime < 0) exposureTime = 0;
       		   OV8865SetExposure(exposureTime);
       		   //printf("\rExposure = %x ", exposureTime);
       	   	   break;}
       	   case 't': {
       		   gain += GAIN_STEP;
       		   if (gain > GAIN_MAX) gain = GAIN_MAX;
       		   OV8865SetGain(gain);
       		   //printf("\rGain = %x ", gain);
       	   	   break;}
       	   case 'g': {
       		   gain -= GAIN_STEP;
       		   if (gain < 0) gain = 0;
       		   OV8865SetGain(gain);
       		   //printf("\rGain = %x ", gain);
       	   	   break;}
       	   case 'r': {
        	   current_focus += manual_focus_step;
        	   if(current_focus >1023) current_focus = 1023;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   //printf("\rFocus = %x ",current_focus);
       	   	   break;}
       	   case 'f': {
        	   if(current_focus > manual_focus_step) current_focus -= manual_focus_step;
        	   OV8865_FOCUS_Move_to(current_focus);
        	   //printf("\rFocus = %x ",current_focus);
       	   	   break;}
       	   case 'y': {
       		   threshold *= 1.02;
			   IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_THRESH, threshold);
			   //printf("\rFocus = %x ",current_focus);
			   break;}
		   case 'h': {
			   threshold /= 1.02;
			   threshold++; // don't let threshold fall to 0, because it wouldn't be increasable by multiplying
			   IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_THRESH, threshold);
			   //printf("\rFocus = %x ",current_focus);
			   break;}
		   case 'u': {
			   pix_threshold += 10;
			   IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_PIXTHRESH, pix_threshold);
			   //printf("\rFocus = %x ",current_focus);
			   break;}
		   case 'j': {
			   pix_threshold -= 10;
			   if (pix_threshold<=0) {
				   pix_threshold = 0; // don't let threshold fall to 0, because it wouldn't be increasable by multiplying
			   }
			   IOWR(EEE_IMGPROC_BASE, EEE_IMGPROC_PIXTHRESH, pix_threshold);
			   //printf("\rFocus = %x ",current_focus);
			   break;}
       }
       //char data[] = "hii!";
       //fwrite(&data, 1, 4, ser);
       //comm_log("Heyyy! %d", 4);

       if (clock() - last_output > OUTPUT_EVERY_CLOCK_TICKS) {
		   comm_log("\r{");
			comm_log("\"search_colour\": %d,", cam_target_colour);
			comm_log("\"found_location_x\": %d,", target_location_x);
			comm_log("\"found_location_y\": %d,", target_location_y);
			comm_log("\"cam_gain\": %d,", gain);
			comm_log("\"cam_exposure\": %d,", exposureTime);
			comm_log("\"cam_focus\": %d,", current_focus);
			comm_log("\"search_sens_threshold\": %d,", threshold);
			comm_log("\"search_pixels_threshold\": %d,", pix_threshold);
			//comm_log("\"target_strength\": %d,", target_strength);
			comm_log("\"found_strength\": %d", target_strength);
			comm_log("}\r");
			last_output = clock();
       }

		char serchar;
		//fread(&thec, 1, 1, ser);
		//serchar = fgetc(ser);
		fflush(ser);

		int nchars = read(read_ser, &serchar, 1);
		//comm_log("char: %x", serchar);
		//comm_log("\n");
		//comm_log("numchars: %x", nchars);
		//comm_log("\n");
		//comm_log("Buf: ");
		//comm_log(patson_sm_key_buffer);
		//comm_log("\n");
		if (nchars != 0 && nchars != 0xffffffff) {//if (serchar != EOF) {
			//comm_log(serchar);
			if (serchar == '\r') {
				//comm_log(serchar);
				// make null-terminated strings so they can be printed
				patson_sm_key_buffer[patson_sm_key_pointer] = 0;
				patson_sm_val_buffer[patson_sm_val_pointer] = 0;
				patson_sm_state = 0;


				char *endptr;
				int val = strtol(patson_sm_val_buffer, &endptr, 16);

				if (strcmp(patson_sm_key_buffer, "search_colour") == 0) {
					//comm_log("\n\nNew target color! %x", val);
					cam_set_target_colour(val);
				} else {
					comm_log("\r{\"error\": \"Unrecognised patson key: '");
					comm_log(patson_sm_key_buffer);
					comm_log("'\"}\r");
				}
				/*comm_log("\r{\"recvd\": \"'");
				comm_log(patson_sm_key_buffer);
				comm_log("' <val> '");
				comm_log(patson_sm_val_buffer);
				comm_log("'\"}\r");*/
				patson_sm_key_pointer = 0;
				patson_sm_val_pointer = 0;
			} else if (patson_sm_state == 0) { // reading key
				if (serchar == ':') {
					patson_sm_state = 1; // now reading value
					patson_sm_val_pointer = 0;
				} else {
					if (patson_sm_key_pointer > PATSON_SM_BUFFER_SIZE-1) {
						patson_sm_key_pointer = 0;
					}
					patson_sm_key_buffer[patson_sm_key_pointer] = serchar;
					patson_sm_key_pointer++;
				}

			} else if (patson_sm_state == 1) {
				if (patson_sm_val_pointer > PATSON_SM_BUFFER_SIZE-1) {
					patson_sm_val_pointer = 0;
				}
				patson_sm_val_buffer[patson_sm_val_pointer] = serchar;
				patson_sm_val_pointer++;
			}
		}




	   //Main loop delay
	   usleep(500);

   };
  return 0;
}
