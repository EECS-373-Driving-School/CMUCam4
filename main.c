
#include "CMUcam4.h"
#include "CMUcom4.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include "drivers/mss_uart/mss_uart.h"
#include <string.h>
#include <stdint.h>
//#include "lcd.h"

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

static const uint8_t RFID[] = "6F005";
static const uint8_t RFID_1[] = "6F005C7D1957";
static const uint8_t RFID_2[] = "6F005CBEB13C";
static const uint8_t RFID_3[] = "6F005C606E3D";
static const uint8_t RFID_4[] = "6F005CBD038D";
static const uint8_t RFID_5[] = "6F005CC0C635";

cmucam4_instance_t cam;
UART_instance_t rfid_uart;

void xbee_rx_handler( void ) {
	//process input data- should be 16bits
	uint8_t rx_buff[16] = {'\0'};
	uint32_t rx_size = 0;
	rx_size = MSS_UART_get_rx( &g_mss_uart1, rx_buff, sizeof(rx_buff) );

}

uint32_t count;
__attribute__ ((interrupt)) void Fabric_IRQHandler( void )
{
	uint8_t rx_buff[16] = {'\0'};
	uint8_t input_buff[30] = {'\0'};

	uint8_t *ptr = input_buff;
	// Check data length
	uint32_t rx_size = 0;

    NVIC_ClearPendingIRQ( Fabric_IRQn );

	int found = 0;
	while (!found) {
		rx_size = UART_get_rx( &rfid_uart, rx_buff, sizeof(rx_buff) );

		// Concatenate input buffer with
		if(rx_size > 0) {
			memcpy(ptr, rx_buff, rx_size);
			for(int i=0; i<rx_size; i++) {
				if(*ptr == '\r')
					found = 1;
				ptr++;
			}
		}
	}

	while (memcmp(input_buff, RFID, sizeof(RFID)-1) != 0) {
		memcpy(input_buff, input_buff+1, ptr-input_buff);
		ptr -= 1;
	}

	if (memcmp(input_buff, RFID_1, sizeof(RFID_1)-1) == 0) {

		while( 1 )
		{
			CMUCam4_cmd(&cam, "TC");
			CMUCam4_copy_T_data( &cam );
			CMUCam4_cmd_no_ack(&cam, ""); //need this to exit from TC

			if(cam.pixels > PIXELS_THRESHOLD && cam.confidence > CONFIDENCE_THRESHOLD) // We see the color to track.
			{
				CMUCam4_cmd(&cam, "L1 10");

				uint8_t xbee_buffer[10] = "Police!";
				MSS_UART_polled_tx_string (&g_mss_uart1, xbee_buffer);

				return;
			}
			else {
				CMUCam4_cmd(&cam, "L0");
			}
		}
	} else {
		CMUCam4_cmd(&cam, "L0");

	}
	return;
}



int main()
{
	//RFID interrupt
	NVIC_EnableIRQ(Fabric_IRQn);

	//Initialize CMUcam
	UART_instance_t cmucam_uart;
	UART_init (
			&cmucam_uart,
			(addr_t)0x40050000,
			UART_19200_BAUD,
			DATA_8_BITS | NO_PARITY);

	CMUCam4_init(&cam, &cmucam_uart);
	char sendCmd[50] = {'\0'};
	sprintf(sendCmd, "ST %d %d %d %d %d %d", RED_MIN, RED_MAX, GREEN_MIN,
			GREEN_MAX, BLUE_MIN, BLUE_MAX);
	CMUCam4_cmd(&cam, sendCmd);

	//Initialize XBee
	MSS_UART_init (
		&g_mss_uart1,
		MSS_UART_9600_BAUD,
		MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY
	);

	MSS_UART_set_rx_handler(
		&g_mss_uart1, //UART Instance being addressed
		xbee_rx_handler, // Pointer to the user defined receive handler function
		MSS_UART_FIFO_SINGLE_BYTE);

	//Initialize RFID
	UART_init (
		&rfid_uart,
		(addr_t)0x40050100,
		UART_9600_BAUD,
		DATA_8_BITS | NO_PARITY
	);

	NVIC_EnableIRQ(Fabric_IRQn);

	while(1);


	return 1;
}
