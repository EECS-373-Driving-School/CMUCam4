
#include "CMUcam4.h"
#include "CMUcom4.h"
#include "drivers/mss_uart/mss_uart.h"
#include <string.h>

#define RED_MIN 23
#define RED_MAX 38
#define GREEN_MIN 51
#define GREEN_MAX 61
#define BLUE_MIN 27
#define BLUE_MAX 48
#define LED_BLINK 1 // 5 Hz
#define WAIT_TIME 5000 // 5 seconds
#define PIXELS_THRESHOLD 40 // The percent of tracked pixels needs to be greater than this 0=0% - 255=100%.
#define CONFIDENCE_THRESHOLD 60 // The percent of tracked pixels in the bounding box needs to be greater than this 0=0% - 255=100%.
#define NOISE_FILTER_LEVEL 2 // Filter out runs of tracked pixels smaller than this in length 0 - 255.
#define YUV_MODE false

#define LED_WRITE_ADDR 0x40050000
cmucam4_instance_t cam;

void setup()
{
	//cmucam4_instance_t *cam = &cmucam4;
	CMUCam4_init(&cam, &g_mss_uart1);
	CMUCam4_begin(&cam);

  // Wait for auto gain and auto white balance to run.

	CMUCam4_LEDOn(&cam, LED_BLINK);

	//CMUCam4_delay(WAIT_TIME);

  // Turn auto gain and auto white balance off.

	CMUCam4_autoGainControl(&cam, false);
	CMUCam4_autoWhiteBalance(&cam, false);

	CMUCam4_LEDOn(&cam, CMUCAM4_LED_ON);

	CMUCam4_colorTracking(&cam, YUV_MODE);

	CMUCam4_noiseFilter(&cam, NOISE_FILTER_LEVEL);
}

void loop()
{
  CMUcam4_tracking_data_t data;

  // Start streaming...
  int *led_addr = (int *)LED_WRITE_ADDR;
  CMUCam4_setAndTrackColor(&cam, RED_MIN, RED_MAX, GREEN_MIN, GREEN_MAX, BLUE_MIN, BLUE_MAX);

  for(;;)
  {
		CMUCam4_getTypeTDataPacket(&cam, &data); // Get a tracking packet at 30 FPS or every 33.33 ms.

    // Process the packet data safely here.

    // More info http://cmucam.org/docs/cmucam4/arduino_api/struct_c_m_ucam4__tracking__data__t.html

    if(data.pixels > PIXELS_THRESHOLD) // We see the color to track.
    {
    	if(data.confidence > CONFIDENCE_THRESHOLD) // The pixels we are tracking are in a dense clump - so maybe they are a single object.
    	{
    		*led_addr = 0;
    	}
    	else
    	{
    		*led_addr = 1;
    	}
    }
    else
    {
    	*led_addr = 1;
    }
  }

  // Do something else here.
}

int main()
{
	setup();
	loop();
	return 1;
}
