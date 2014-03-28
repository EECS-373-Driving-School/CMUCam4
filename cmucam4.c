#include "cmucam4.h"
#include "drivers/mss_uart/mss_uart.h"
#include <string.h>

cmucam4_instance_t cmucam4;

static const uint8_t CMUCAM4_ACK[] = {'A', 'C', 'K', '\r'};
static const uint8_t CMUCAM4_NAK[] = {'N', 'A', 'K', '\r'};
static const uint8_t CMUCAM4_ERR[] = {'E', 'R', 'R', '\r'};

void CMUCam4_init ( cmucam4_instance_t *cam, mss_uart_instance_t *uart)
{
	// Associate CMUCam with UART connection
	cam->uart = uart;

	// Zero-out buffers
	CMUCam4_flush_buffers(cam);

	// Initialize UART at low speed mode (19200 baud)
	MSS_UART_init (
		cam->uart,
		MSS_UART_19200_BAUD,
		MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT
	);

	// Set baud to high-speed
	CMUCam4_cmd_no_ack (cam, "BM", "250000");

	// Re-initialize UART at high speed mode (250000 baud)
	MSS_UART_init (
		cam->uart,
		250000,
		MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT
	);

	// Zero-out buffers
	CMUCam4_flush_buffers(cam);
}

void CMUCam4_flush_buffers ( cmucam4_instance_t *cam )
{
	// Clear the UART buffer
	while (CMUCam4_read(cam) != 0);

	// Zero the buffers
	memset(cam->cmd_buffer, 0, CMUCAM4_CMD_BUFFER_SIZE);
	memset(cam->input_buffer, 0, CMUCAM4_INPUT_BUFFER_SIZE);
	memset(cam->data_buffer, 0, CMUCAM4_INPUT_BUFFER_SIZE);
}

int CMUCam4_cmd_no_ack ( cmucam4_instance_t *cam, const char *cmd, const char *args )
{
	// Make sure cmd + args is < CMUCAM4_CMD_BUFFER_SIZE
	if (strlen(cmd) + strlen(args) < CMUCAM4_CMD_BUFFER_SIZE - 3)
		return CMUCAM4_TX_CMD_TOO_LONG;

	// Format command string
	sprintf(cam->cmd_buffer, "%s %s\r", cmd, args);

	// Transmit string (blocking)
	// XXX: consider making this async
	MSS_UART_polled_tx_string (cam->uart, cam->cmd_buffer);

	return CMUCAM4_TX_CMD_OK;
}

int CMUCam4_cmd ( cmucam4_instance_t *cam, const char *cmd, const char *args )
{
	// Send command
	CMUCam4_cmd_no_ack(cam, cmd, args);

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
		size_t rx_len;

		// Read data into buffer
		rx_len = CMUCam4_read(cam);

		// Concatenate input buffer with
		memcpy(cam->data_buffer, cam->input_buffer, rx_len);
		ptr += rx_len;

		// Check data length
		if (ptr - cam->data_buffer < sizeof(CMUCAM4_ACK))
			continue;

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
			memcpy(cam->data_buffer, cam->data_buffer + 1, ptr - cam->data_buffer - 1);
			ptr -= 1;
			continue;
		}

		break;
	}

	// Remove first four characters from data buffer
	memcpy(cam->data_buffer, cam->data_buffer + 4, ptr - cam->data_buffer - 4);

	return retval;
}

size_t CMUCam4_read ( cmucam4_instance_t *cam )
{
	return MSS_UART_get_rx(cam->uart, cam->input_buffer, CMUCAM4_INPUT_BUFFER_SIZE);
}
