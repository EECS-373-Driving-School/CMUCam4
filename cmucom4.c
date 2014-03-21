/*
 * cmucom4.c
 *
 *  Created on: Mar 20, 2014
 *      Author: nick
 */

#include "cmucom4.h"
#include "drivers/mss_uart/mss_uart.h"

cmucom4_instance_t cmucom4_1;
const cmucom4_exception_t CMUCOM4_NOT_IMPLEMENTED = 1;

void CMUcom4_init ( cmucom4_instance_t *cmucom4, mss_uart_instance_t* uart )
{
	cmucom4->buffer = calloc(CMUCOM_RX_BUFFER_SIZE, sizeof(uint8_t));
	cmucom4->cur_elem = cmucom4_buffer;
	cmucom4->uart = uart;
}

void CMUcom4_begin ( cmucom4_instance_t *cmucom4 )
{
	MSS_UART_init
	(
		cmucom4->uart,
		MSS_UART_19200_BAUD,
		MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT
	);
}

void CMUcom4_end ( cmucom4_instance_t *cmucom4 ) { }

uint8_t CMUcom4_read ( cmucom4_instance_t *cmucom4 )
{
	if (MSS_UART_get_rx(cmucom4->uart, cmucom4->buffer, CMUCOM_RX_BUFFER_SIZE) == 0)
		return -1;

	return *cmucom4->buffer;
}

void CMUcom4_write ( cmucom4_instance_t *cmucom4, uint8_t *data, size_t size )
{
	MSS_UART_polled_tx(cmucom4->uart, data, size);
}

void CMUcom4_write ( cmucom4_instance_t *cmucom4, const char *data )
{
	MSS_UART_polled_tx_string(cmucom4->uart, data);
}

void CMUcom4_write ( cmucom4_instance_t *cmucom4, uint8_t data )
{
	CMUcom4_write(cmucom4, &data, sizeof(data));
}

int CMUcom4_available ( cmucom4_instance_t *cmucom4 )
{
	throw CMUCOM4_NOT_IMPLEMENTED;
	return -1;
}

unsigned long CMUcom4_milliseconds ( cmucom4_instance_t *cmucom4 )
{
	throw CMUCOM4_NOT_IMPLEMENTED;
	return -1;
}
