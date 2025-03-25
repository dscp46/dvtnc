#ifndef __SERIAL_H
#define __SERIAL_H

#include <file.h>
#include <stdint.h>

typedef serial_t int8_t;

typedef serial_stats_t {
	uintmax_t sent_bytes;
	uintmax_t recv_bytes;
} serial_stats_t;

const size_t SERIAL_TX_BUFSIZE = 8640;	// 18s of Tx at 3,840bps (DV Fast data)
const size_t SERIAL_RX_BUFSIZE = 2050;	// Worst case yEncoded frame: 1 start byte, 1024*2 escaped chars and 1 stop byte

// Open port
serial_t open(void);

// Close port
void close( serial_t port);

// Send data
int16_t send( serial_t port, void *buf, size_t len)

// 
uint16_t get_outstanding_tx_bytes( serial_t port)

// Receive data
int16_t recv( serial_t port, void **buf, size_t buf_size)

#endif	/* __SERIAL_H */
