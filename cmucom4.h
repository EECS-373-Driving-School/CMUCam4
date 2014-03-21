/*
 * cmucom4.h
 *
 *  Created on: Mar 20, 2014
 *      Author: njurgens@umich.edu
 */

#ifndef CMUCOM4_H_
#define CMUCOM4_H_

#include "drivers/mss_uart/mss_uart.h"

#define CMUCOM_RX_BUFFER_SIZE 1

struct CMUcom4
{
	unit8_t *buffer;
	mss_uart_instance_t* uart;
};
typedef CMUcom4 cmucom4_instance_t;

extern cmucom4_instance_t cmucom4_1;

/**
 * Initialize the CMUcom4 object to use the given UART port
 */
void CMUcom4_init ( cmucom4_instance_t*, mss_uart_instance_t* );

/**
 * Open serial connection
 */
void CMUcom4_begin ( cmucom4_instance_t* );

/**
 * Close the serial connection
 */
void CMUcom4_end ( cmucom4_instance_t* );

/**
 * Read the first byte of incoming serial data
 */
uint8_t CMUcom4_read ( cmucom4_instance_t* );

/**
 * Write contents of buffer to device
 */
size_t CMUcom4_write ( cmucom4_instance_t*, const unint8_t*, size_t );

/**
 * Write null-terminated string to device
 */
size_t CMUcom4_write ( cmucom4_instance_t*, const char* );

/**
 * Write a single byte to device
 */
size_t CMUcom4_write ( cmucom4_instance_t*, uint8_t );

/**
 * Number of bytes available to be read
 */
int CMUcom4_available ( cmucom4_instance_t*, uint8_t );

/**
 * Number of milliseconds since program started
 */
unsigned long CMUcom4_milliseconds ( cmucom4_instance_t* );

typedef uint8_t cmucom4_exception_t;
extern const cmucom4_exception_t CMUCOM4_NOT_IMPLEMENTED;

#endif /* CMUCOM4_H_ */
