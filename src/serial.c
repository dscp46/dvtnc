#include "serial.h"
#include "ringbuffer.h"

#include <errno.h>
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
	
	// Pre-existing serial settings
	struct termios serial_settings;
	
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

// Internal definitions
int8_t serial_restore( serial_t portnum, FILE *fd);
int8_t serial_save( serial_t portnum, FILE *fd);
int8_t serial_configure( serial_t portnum, speed_t speed);

// Allocate a serial port and return an opaque pointer. Return a negative value if allocation failed.
serial_t allocate_serial()
{
	for( size_t i=0; i < SERIAL_PORT_MAX; ++i)
	{
		if ( __port_list[i].used == false )
		{
			__port_list[i].used = true;
			// Initialize conds and mutexes
			pthread_mutex_init( &__port_list[i].xoff_mutex, NULL);
			pthread_cond_init( &__port_list[i].xoff_condition, NULL);

			// Initialize ringbuffers
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

	pthread_mutex_destroy( &__port_list[port].xoff_mutex );
	pthread_cond_destroy( &__port_list[port].xoff_condition );

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

		while ( ringbuffer_bytes_available( serial->tx_buffer) == 0 ) {
		}

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
serial_t serial_open( const char* fname, speed_t speed)
{	
	if ( fname == NULL )
		return __INVALID_SERIAL_PORT;

	FILE* fport = fopen( fname, "r+"); 
	
	if ( fport == NULL )
		return __INVALID_SERIAL_PORT;
	
	serial_t portnum = allocate_serial();
	__port_list[portnum].port = fport;
	serial_configure( portnum, speed);
	
	return portnum;
}

// Close port
void serial_close( serial_t portnum)
{
	fclose( __port_list[portnum].port);
	free_serial( portnum);
}

// Send data
size_t serial_send( serial_t portnum, void *buf, size_t len)
{
	if ( portnum < 0 || portnum >= SERIAL_PORT_MAX || __port_list[portnum].used == false)
		return 0;

	return fwrite( buf, len, 1, __port_list[portnum].port);
}

// Receive data
size_t serial_recv( serial_t portnum, void *buf, size_t buf_size)
{
	if ( portnum < 0 || portnum >= SERIAL_PORT_MAX || __port_list[portnum].used == false)
		return 0;

	return fread( buf, buf_size, 1, __port_list[portnum].port);
}

int8_t serial_save( serial_t portnum, FILE *fd)
{
	if( tcgetattr( fileno(fd), &__port_list[portnum].serial_settings) != 0)
	{
		fprintf( stderr, "Error when saving serial device settings:  %s\n", strerror( errno));
		return 1;
	}
	return 0;
}

int8_t serial_restore( serial_t portnum, FILE *fd)
{
	if( tcsetattr( fileno(fd), TCSANOW, &__port_list[portnum].serial_settings) != 0)
	{
		fprintf( stderr, "Error when restoring serial device settings: %s\n", strerror( errno));
		return 1;
	}
	return 0;
}

int8_t serial_configure( serial_t portnum, speed_t speed)
{
	struct termios stty;
	FILE *fd = __port_list[portnum].port;
		
	if( tcgetattr( fileno(fd), &__port_list[portnum].serial_settings) != 0)
	{
		fprintf( stderr, "Error when saving serial device settings: %s\n", strerror( errno));
		return 1;
	}

	// Set port speed
	cfsetispeed( &stty, speed);
	cfsetospeed( &stty, speed);

	// Set 8N1 and disable hardware flow control
	stty.c_cflag &= ~( PARENB | CSTOPB | CSIZE | CRTSCTS);
	stty.c_cflag |= CS8 | CREAD | CLOCAL;

	// Disable software flow control and disable special chars handling
	stty.c_lflag &= ~( ICANON | ECHO | ECHOE | ECHONL | ISIG );
	stty.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF| IXANY);

	stty.c_oflag &= ~( OPOST | ONLCR);
	
	// Set blocking IO with no timeout
	stty.c_cc[VTIME] = 0;
	stty.c_cc[VMIN] = 1;

	if( tcsetattr( fileno(fd), TCSANOW, &stty) != 0)
	{
		fprintf( stderr, "Error configuring serial port: %s\n", strerror( errno));
		return 1;
	}

	return 0;
}
