#include <string.h>
#include <unistd.h>

#include "serial.h"

// Structure that holds the serial port properties
typedef struct serial_s {
	bool  used;
	FILE* socket;
	char rx_buffer[SERIAL_TX_BUFSIZE];
	char tx_buffer[SERIAL_RX_BUFSIZE];
	
	char *cur_tx_read;
	char *cur_tx_write;
	
	char *cur_rx_read;
	
	bool xoff                      = false
	pthread_mutex_t xoff_mutex     = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t  xoff_condition = PTHREAD_COND_INITIALIZER;
	
	pthread_mutex_t txbuff_mutex   = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t  txbuff_cond    = PTHREAD_COND_INITIALIZER;
	
	pthread_t tx_thread;
	pthread_t rx_thread;
	
	serial_stats_t stats;
} serial_s;

// Max number of serial ports
const size_t SERIAL_PORT_MAX = 8;

// Opaque structure which holds the serial port properties
serial_s __port_list[SERIAL_PORT_MAX];

// The invalid opaque pointer, useful to report failed allocation
const serial_t __INVALID_SERIAL_PORT = -1;

// Allocate a serial port and return an opaque pointer. Return a negative value if allocation failed.
serial_t allocate_serial()
{
	for( size_t i=0; i < SERIAL_PORT_MAX; ++i)
	{
		if ( __port_list[i].used == false )
		{
			__port_list[i].used == true;
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

	// Mark the port as unused
	__port_count[port].used = false;		
}

/***** Threads *****/
void *serial_writer(void *arg)
{ 
	serial_s *serial = (serial_s*) arg;
	char buf[1];
	
	while (1) {
		// Block thread if XOFF is asserted
		pthread_mutex_lock( &serial->xoff_mutex);
		while ( serial->xoff ) {
			pthread_cond_wait( &serial->xoff_condition, &serial->xoff_mutex);
		}
		pthread_mutex_unlock( &serial->xoff_mutex);
		
		// Sleeplock as long as the ring buffer is empty
		pthread_mutex_lock( &serial->txbuff_mutex)
		while ( serial->cur_tx_read == cur_tx_write) {
			pthread_cond_wait( &serial->txbuff_condition, &serial->txbuff_mutex);
		}
		pthread_mutex_unlock( &serial->txbuff_mutex)
		
		// Write one byte from ring buffer
		serial->cur_tx_read = ring_memcpy( &buf, serial->cur_tx_read, 1, serial->tx_buffer, SERIAL_TX_BUFSIZE);
	}
	return NULL;
}

void *serial_reader( void *arg)
{
	// TODO: read a few bytes at once, define buffer size
	serial_s *serial = (serial_s*) arg;
	char buf[1];
	
	while (1) {
		// Read 1 byte from serial port
		read( serial->socket, &buf, 1);
		
		switch ( c )
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
		}
	}
	return NULL;
}

// Open serial port, returns an opaque designator.
serial_t open( void)
{
	return __INVALID_SERIAL;
}

// Close port
void close( serial_t port);


// Send data
int16_t send( serial_t port, void *buf, size_t len)
{
}

// Receive data
int16_t recv( serial_t port, void **buf, size_t buf_size)
{

}
