#include "serial.h"
#include "ringbuffer.h"

#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

// Structure that holds the serial port properties
typedef struct serial_s {
	bool  used;
	FILE* port;
	ringbuffer_t *rx_buffer;
	ringbuffer_t *tx_buffer;
	
	bool xoff;
	pthread_mutex_t xoff_mutex;
	pthread_cond_t  xoff_condition;
	
	pthread_mutex_t txbuff_mutex;
	pthread_cond_t  txbuff_cond;
	
	pthread_t tx_thread;
	pthread_t rx_thread;
	
	serial_stats_t stats;
} serial_s;

// Max number of serial ports
#define SERIAL_PORT_MAX 8

// Opaque structure which holds the serial port properties
serial_s __port_list[SERIAL_PORT_MAX];

// The invalid opaque pointer, useful to report failed allocation
const serial_t __INVALID_SERIAL_PORT = -1;

#define XON	0x11
#define	XOFF	0x13

// Allocate a serial port and return an opaque pointer. Return a negative value if allocation failed.
serial_t allocate_serial()
{
	for( size_t i=0; i < SERIAL_PORT_MAX; ++i)
	{
		if ( __port_list[i].used == false )
		{
			__port_list[i].used = true;
			// FIXME Initialize conds and mutexes
			// FIXME Initialize ringbuffers
			__port_list[i].tx_buffer = ringbuffer_alloc( SERIAL_TX_BUFSIZE);
			__port_list[i].rx_buffer = ringbuffer_alloc( SERIAL_RX_BUFSIZE);
			return i;
		}
	}
	return __INVALID_SERIAL_PORT;
}

void free_serial( serial_t port)
{
	// Do nothing if:
	//	- The opaque pointer is invalid (<0)
	//	- The opaque pointer is out of bounds. (Segfault handling for later?)
	if ( port < 0 || port >= SERIAL_PORT_MAX )
		return;

	ringbuffer_free( __port_list[port].rx_buffer );
	ringbuffer_free( __port_list[port].tx_buffer );
	// Mark the port as unused
	__port_list[port].used = false;		
}

/***** Threads *****/
void *serial_writer(void *arg)
{ 
	serial_s *serial = (serial_s*) arg;
	unsigned char buf;
	
	while (1) {
		// Block thread if XOFF is asserted
		pthread_mutex_lock( &serial->xoff_mutex);
		while ( serial->xoff ) {
			pthread_cond_wait( &serial->xoff_condition, &serial->xoff_mutex);
		}
		pthread_mutex_unlock( &serial->xoff_mutex);
		
		// Sleeplock as long as the ring buffer is empty
		pthread_mutex_lock( &serial->txbuff_mutex);
		while ( ringbuffer_bytes_available( serial->tx_buffer) == 0 ) {
			pthread_cond_wait( &serial->txbuff_cond, &serial->txbuff_mutex);
		}
		pthread_mutex_unlock( &serial->txbuff_mutex);
		
		// Write one byte from ring buffer
		buf = ringbuffer_pull( &buf, 1, serial->tx_buffer);
	
		fwrite( &buf, 1, 1, serial->port);
	}
	return NULL;
}

void *serial_reader( void *arg)
{
	// TODO: read a few bytes at once, define buffer size
	serial_s *serial = (serial_s*) arg;
	unsigned char buf;
	
	while (1) {
		// Read 1 byte from serial port
		fread( &buf, 1, 1, serial->port);
		
		switch ( buf )
		{
		case XOFF:
			pthread_mutex_lock( &serial->xoff_mutex);
			serial->xoff = true;
			pthread_mutex_unlock( &serial->xoff_mutex);
			continue;
			
		case XON:
			pthread_mutex_lock( &serial->xoff_mutex);
			serial->xoff = false;
			pthread_mutex_unlock( &serial->xoff_mutex);
			continue;
			
		default:
			// TODO: Add char to read buffer
			ringbuffer_push( &buf, 1, serial->rx_buffer);
		}
	}
	return NULL;
}

// Open serial port, returns an opaque designator.
serial_t serial_open( void)
{
	return __INVALID_SERIAL_PORT;
}

// Close port
void serial_close( serial_t port);


// Send data
int16_t serial_send( serial_t port, void *buf, size_t len)
{
}

// Receive data
int16_t serial_recv( serial_t port, void **buf, size_t buf_size)
{

}
