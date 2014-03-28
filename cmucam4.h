#ifndef CMUCAM4_H_
#define CMUCAM4_H_

#include <string.h>
#include "drivers/mss_uart/mss_uart.h"

#define CMUCAM4_CMD_BUFFER_SIZE 256
#define CMUCAM4_DATA_BUFFER_SIZE 256
#define CMUCAM4_INPUT_BUFFER_SIZE 256

typedef struct {
	mss_uart_instance_t *uart;
	uint8_t cmd_buffer[CMUCAM4_CMD_BUFFER_SIZE];
	uint8_t input_buffer[CMUCAM4_INPUT_BUFFER_SIZE];
	uint8_t data_buffer[CMUCAM4_DATA_BUFFER_SIZE];
	size_t data_size;
} cmucam4_instance_t;

extern cmucam4_instance_t cmucam4;

/**
 * Initialize CMUCam4 object
 *
 * @param[in] cam Pointer to CMUCam4 object
 * @param[in] uart Pointer to MSS UART object
 */
void CMUCam4_init ( cmucam4_instance_t *cam, mss_uart_instance_t *uart );

/**
 * Send command to CMUCam4. Waits for ACK.
 *
 * XXX: There is no timeout mechanism right now. If there is no device attached, your program
 *      will hang forever!
 *
 * @param[in] cam Pointer to CMUCam4 object
 * @param[in] cmd Command string
 * @param[in] args Arguments string
 *
 * @returns Response to command (ACK, NAK, ERR)
 */
int CMUCam4_cmd ( cmucam4_instance_t *cam, const char *cmd, const char *args );

/**
 * Send command to CMUCam4. Does not wait for ACK.
 *
 * @param[in] cam Pointer to CMUCam4 object
 * @param[in] cmd Command string
 * @param[in] args Arguments string
 *
 * @retval CMUCAM4_TX_CMD_OK No problem transmitting command
 * @retval CMUCAM4_TX_CMD_TOO_LONG Command exceeds output buffer size
 */
int CMUCam4_cmd_no_ack ( cmucam4_instance_t *cam, const char *cmd, const char *args );

/**
 * Read in data until an ACK, NAK, or ERR is encountered.
 *
 * @param[in] cam Pointer to CMUCam4 object
 *
 * @retval CMUCAM4_READ_ACK Received ACK
 * @retval CMUCAM4_READ_NAK Received NAK
 * @retval CMUCAM4_READ_ERR Received ERR
 */
int CMUCam4_wait_for_ack ( cmucam4_instance_t *cam );

/**
 * Reads data from UART into CMUCam4 input buffer.
 *
 * @param[in] cam Pointer for CMUCam4 object
 * @param[in] dest Destination
 * @param[in] size Input buffer length
 *
 * @returns Number of bytes read
 */
size_t CMUCam4_copy_data ( cmucam4_instance_t *cam, uint8_t *dest, size_t dest_size );

void CMUCam4_read_H ( cmucam4_instance_t *cam, char *str, size_t len );

void CMUCam4_read_F ( cmucam4_instance_t *cam, char *str, size_t len );

void CMUCam4_read_S ( cmucam4_instance_t *cam, char *str, size_t len );

void CMUCam4_read_T ( cmucam4_instance_t *cam, char *str, size_t len );

void CMUCam4_flush_buffers ( cmucam4_instance_t *cam );

#define CMUCAM4_TX_CMD_OK 0
#define CMUCAM4_TX_CMD_TOO_LONG 1

#define CMUCAM4_READ_ACK 0
#define CMUCAM4_READ_NAK 1
#define CMUCAM4_READ_ERR 2

#endif
