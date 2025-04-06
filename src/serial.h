#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>
#include <stdint.h>
#include <termios.h>

//	Opaque serial port type
typedef int8_t serial_t;

typedef struct serial_stats_t {
	uintmax_t sent_bytes;
	uintmax_t recv_bytes;
} serial_stats_t;

// Callback function pointer type
typedef void (*serial_callback_t)( void*, size_t);

// 18s of Tx at 3,840bps (DV Fast data)
#define SERIAL_TX_BUFSIZE 8640

// Worst case yEncoded frame: 1 start byte, 1024*2 escaped chars and 1 stop byte
#define SERIAL_RX_BUFSIZE 2050

// Open port
serial_t serial_open( const char* fname, speed_t speed, serial_callback_t process_rx_data);

// Close port
void serial_close( serial_t port);

// Update the Rx data processor
void serial_update_rx_processor( serial_t port, serial_callback_t process_rx_data);

// Send data
size_t serial_send( serial_t portnum, void *buf, size_t len);

// 
uint16_t serial_get_outstanding_tx_bytes( serial_t port);

// Receive data
size_t serial_recv( serial_t portnum, void *buf, size_t buf_size);

#endif	/* __SERIAL_H */
