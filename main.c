
#include "CMUcam4.h"
#include "CMUcom4.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include <string.h>

#define RED_MIN 21
#define RED_MAX 33
#define GREEN_MIN 37
#define GREEN_MAX 51
#define BLUE_MIN 33
#define BLUE_MAX 50
#define LED_BLINK 1 // 5 Hz
#define WAIT_TIME 5000 // 5 seconds
#define PIXELS_THRESHOLD 10 // The percent of tracked pixels needs to be greater than this 0=0% - 255=100%.
#define CONFIDENCE_THRESHOLD 20 // The percent of tracked pixels in the bounding box needs to be greater than this 0=0% - 255=100%.
#define NOISE_FILTER_LEVEL 2 // Filter out runs of tracked pixels smaller than this in length 0 - 255.
#define YUV_MODE false

int main()
{
	cmucam4_instance_t cam;
	char str[50] = {'\0'};

	UART_instance_t mss_uart;
	CMUCam4_init(&cam, &mss_uart);

	char sendCmd[50] = {'\0'};
	sprintf(sendCmd, "ST %d %d %d %d %d %d", RED_MIN, RED_MAX, GREEN_MIN,
			GREEN_MAX, BLUE_MIN, BLUE_MAX);
	CMUCam4_cmd(&cam, sendCmd);

	while( 1 )
	{
		CMUCam4_cmd(&cam, "TC");

		CMUCam4_copy_T_data( &cam );

		printf("\r\nstring %d %d %d %d %d %d %d %d\r\n", cam.mx, cam.my, cam.x1, cam.y1,
				cam.x2, cam.y2, cam.pixels, cam.confidence);

		CMUCam4_cmd_no_ack(&cam, ""); //need this to exit from TC

		if(cam.pixels > PIXELS_THRESHOLD && cam.confidence > CONFIDENCE_THRESHOLD) // We see the color to track.
		{
			CMUCam4_cmd(&cam, "L1 10");
		}
		else {
			CMUCam4_cmd(&cam, "L0");

		}
	}
	return 1;
}
