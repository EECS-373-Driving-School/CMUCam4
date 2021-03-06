#include "CMUcam4.h"
#include "drivers/CoreUARTapb/core_uart_apb.h"
#include <string.h>
#include <math.h>
#include <assert.h>

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

cmucam4_instance_t cmucam4;

static const uint8_t CMUCAM4_ACK[] = {'A', 'C', 'K', '\r'};
static const uint8_t CMUCAM4_NAK[] = {'N', 'A', 'K', '\r'};
static const uint8_t CMUCAM4_ERR[] = {'E', 'R', 'R', '\r'};

void CMUCam4_init ( cmucam4_instance_t *cam, UART_instance_t *uart)
{
	// Associate CMUCam with UART connection
	cam->uart = uart;

	// Initialize UART at low speed mode (19200 baud)


//	// Set baud to high-speed
//	CMUCam4_cmd(cam, "BM", "250000");
//
//	// Re-initialize UART at high speed mode (250000 baud)
//	UART_init (
//		cam->uart,
//		250000,
//		UART_DATA_8_BITS | UART_NO_PARITY | UART_ONE_STOP_BIT
//	);
//
//	int foo = CMUCam4_cmd(cam, "", "");

	// Zero-out buffers
	CMUCam4_flush_buffers(cam);
}

void CMUCam4_flush_buffers ( cmucam4_instance_t *cam )
{
	// Clear the UART buffer
	//while ( != 0);
	UART_get_rx(cam->uart, cam->input_buffer, CMUCAM4_INPUT_BUFFER_SIZE);

	// Zero the buffers
	cam->data_size = 0;
	memset(cam->cmd_buffer, 0, CMUCAM4_CMD_BUFFER_SIZE);
	memset(cam->input_buffer, 0, CMUCAM4_INPUT_BUFFER_SIZE);
	memset(cam->data_buffer, 0, CMUCAM4_INPUT_BUFFER_SIZE);
}

int CMUCam4_cmd_no_ack ( cmucam4_instance_t *cam, const char *cmd )
{
	// Make sure cmd + args is < CMUCAM4_CMD_BUFFER_SIZE
	CMUCam4_flush_buffers(cam);

	if (strlen(cmd) > CMUCAM4_CMD_BUFFER_SIZE - 3)
		return CMUCAM4_TX_CMD_TOO_LONG;

	// Format command string
	sprintf(cam->cmd_buffer, "%s\r", cmd);

	// Transmit string (blocking)
	// XXX: consider making this async
	UART_polled_tx_string (cam->uart, cam->cmd_buffer);

	return CMUCAM4_TX_CMD_OK;
}

int CMUCam4_cmd ( cmucam4_instance_t *cam, const char *cmd )
{
	// Send command
	CMUCam4_cmd_no_ack(cam, cmd);

	// Listen for ACK/NAK response
	return CMUCam4_wait_for_ack(cam);
}

int CMUCam4_wait_for_ack ( cmucam4_instance_t *cam )
{
	uint8_t *ptr;
	int retval;

	memset(cam->data_buffer, 0, CMUCAM4_INPUT_BUFFER_SIZE);
	ptr = cam->data_buffer;

	while (1)
	{

		// Check data length
		while (ptr - cam->data_buffer < 4) {
			size_t rx_len = 0;

			// Read data into buffer
			rx_len = UART_get_rx(cam->uart, cam->input_buffer, CMUCAM4_INPUT_BUFFER_SIZE);

			// Concatenate input buffer with
			if(rx_len > 0) {
				memcpy(ptr, cam->input_buffer, rx_len);
				ptr += rx_len;
			}
		}

		// Response is ACK
		if (memcmp(cam->data_buffer, CMUCAM4_ACK, sizeof(CMUCAM4_ACK)) == 0)
			retval = CMUCAM4_READ_ACK;

		// Response is NAK
		else if (memcmp(cam->data_buffer, CMUCAM4_NAK, sizeof(CMUCAM4_NAK)) == 0)
			retval = CMUCAM4_READ_NAK;

		// Response is ERR
		else if (memcmp(cam->data_buffer, CMUCAM4_ERR, sizeof(CMUCAM4_ERR)) == 0)
			retval = CMUCAM4_READ_ERR;

		// No match
		else {
			// Discard first character and data by one
			memcpy(cam->data_buffer, cam->data_buffer + 1, ptr - cam->data_buffer);
			ptr -= 1;
			continue;
		}

		break;
	}

	// Remove first four characters from data buffer
	memcpy(cam->data_buffer, cam->data_buffer + 4, MAX(ptr - cam->data_buffer - 4, 5));

	cam->data_size = ptr - cam->data_buffer - 4; //TODO also minus 4?

	return retval;
}

size_t CMUCam4_copy_T_data( cmucam4_instance_t *cam )
{
	uint8_t *ptr, *firstBound;
	int numInput = 0;

	ptr = cam->data_buffer;
	firstBound = ptr;

	while (1)
	{
		char temp[5] = {'\0'};
		if (*ptr == '\r') {
			memcpy(temp, firstBound+1, ptr-firstBound-1);
			cam->confidence = atoi(temp);
			break;
		}
		if(*ptr == ' ') {

			switch(numInput) {
				case 1: memcpy(temp, firstBound+1, ptr-firstBound-1);
					cam->mx = atoi(temp);
					break;
				case 2: memcpy(temp, firstBound+1, ptr-firstBound-1);
					cam->my = atoi(temp);
					break;
				case 3: memcpy(temp, firstBound+1, ptr-firstBound-1);
					cam->x1 = atoi(temp);
					break;
				case 4: memcpy(temp, firstBound+1, ptr-firstBound-1);
					cam->y1 = atoi(temp);
					break;
				case 5: memcpy(temp, firstBound+1, ptr-firstBound-1);
					cam->x2 = atoi(temp);
					break;
				case 6: memcpy(temp, firstBound+1, ptr-firstBound-1);
					cam->y2 = atoi(temp);
					break;
				case 7: memcpy(temp, firstBound+1, ptr-firstBound-1);
					cam->pixels = atoi(temp);
					break;
			}
			firstBound = ptr;
			numInput++;
		}
		if(cam->data_size > 0)
			ptr++;

		if (ptr >= cam->data_buffer + cam->data_size) {
			size_t rx_len = 0;

			// Read data into buffer
			while(rx_len == 0){
				rx_len = UART_get_rx(cam->uart, cam->input_buffer, CMUCAM4_INPUT_BUFFER_SIZE);
			}
			// Check for buffer overrun
			//if (ptr - cam->data_buffer + rx_len > CMUCAM4_INPUT_BUFFER_SIZE)
			//	return 0;

			// Concatenate input buffer with
			//TODO probably only need first 30 chars
			memcpy(ptr, cam->input_buffer, MIN(rx_len, CMUCAM4_INPUT_BUFFER_SIZE - cam->data_size));
			cam->data_size += rx_len;
		}
	}
	return 1;

}

size_t CMUCam4_copy_data ( cmucam4_instance_t *cam, uint8_t *dest, size_t dest_size )
{
	uint8_t *ptr;
	size_t size;

	ptr = cam->data_buffer;

	while (1)
	{
		if (*ptr == ':')
			break;
		if(cam->data_size > 0)
			ptr++;

		if (ptr >= cam->data_buffer + cam->data_size) {
			size_t rx_len;

			// Read data into buffer
			rx_len = UART_get_rx(cam->uart, cam->input_buffer, CMUCAM4_INPUT_BUFFER_SIZE);

			// Check for buffer overrun
			if (ptr - cam->data_buffer + rx_len >= CMUCAM4_INPUT_BUFFER_SIZE)
				return 0;

			// Concatenate input buffer with
			memcpy(ptr, cam->input_buffer, rx_len);
			cam->data_size += rx_len;
		}
	}
	size = ptr - cam->data_buffer;
	printf("return data %s\r\n", cam->data_buffer);
	// TODO: check against dest_size
	memcpy(dest, cam->data_buffer, MIN(size, dest_size));

	return size;
}

