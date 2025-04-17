
#include "./kiss.h"
#include "../dse.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*** Internal definitions ***/

// TCP Buffer size
#define BUFFER_SIZE 1460

// AX.25 frame: 1024b, Start & End markers: 2b, CRC32: 4b, Type: 1b 
#define RX_MTU 1031

typedef struct kiss_iface_attrs {

	// Client socket file descriptor
	int client_fd;
	
	// Remote client address
	struct sockaddr* client_addr;
	
	// Remote client address size
	size_t client_addr_sz;
	
	// Client handler thread
	pthread_t handler_thread;
	
	// Parent dse
	dse *sw;
} kiss_iface_attrs;



void kiss_send( ax_iface *self, void *frame, size_t len);
void *kiss_client_handler( void* arg);
void kiss_recv( ax_iface *self, unsigned char type, const void *frame, size_t len);
void kiss_print_frame( const void *frame, size_t len);

/*** General functions ***/
ax_iface *kiss_ifup( int client_fd, struct sockaddr* client_addr, size_t client_addr_sz)
{
	ax_iface *iface = (ax_iface*) malloc( sizeof( ax_iface));
	if( iface == NULL )
		return NULL;

	// Internal attributes section
	iface->p = NULL;
	
	kiss_iface_attrs *attrs = (kiss_iface_attrs*) malloc(sizeof(kiss_iface_attrs));
	if ( attrs == NULL )
	{
		kiss_ifdown( iface);
		return NULL;
	}
	
	attrs->client_fd = client_fd;
	attrs->client_addr = client_addr;
	attrs->client_addr_sz = client_addr_sz;
	attrs->sw = NULL;
	iface->p = attrs;
	
	iface->send_frame = kiss_send;
	
	// Start handler thread
	if( pthread_create( &attrs->handler_thread, NULL, kiss_client_handler, iface) != 0) {
		printf("Thread creation failed.\n");
		kiss_ifdown( iface);
		return NULL;
	}
	
	// TODO: Set protocol state UP
		
	return iface;
}

void kiss_ifdown( ax_iface *iface)
{
	if ( iface == NULL )
		return;
	
	kiss_iface_attrs* attrs = (kiss_iface_attrs*) iface->p;
	if ( attrs != NULL )
	{
		// Stop handler thread
		pthread_cancel( attrs->handler_thread);
		
		// TODO: Set protocol state DOWN
	
		close( attrs->client_fd);
		
		if ( attrs->client_addr != NULL )
			free( attrs->client_addr);
		
		free( attrs);
	}
	
	free( iface);
}

size_t kiss_encoded_size( const void* frame, size_t len)
{
	if ( frame == NULL )
		return 0;

	// Include head and tail FENDs, plus the data type
	size_t result = 3;
	unsigned char *src = (unsigned char*)frame;

	while ( len-- > 0 )
	{
		if ( *src == FEND || *src == FESC )
			++result;

		++result;
		++src;
	}

	return result;
}

void kiss_encode( const void *in, size_t len, unsigned char type, void *out)
{
	if ( in == NULL || out == NULL )
		return;
	
	unsigned char *src = (unsigned char*)in;
	unsigned char *dst = (unsigned char*)out;
	
	*dst++ = FEND;
	*dst++ = type;
	while( len-- > 0 )
	{
		switch ( (unsigned int)*src )
		{
		case FEND:
			*dst++ = FESC;
			*dst++ = TFEND;
			++src;
			break;

		case FESC:
			*dst++ = FESC;
			*dst++ = TFESC;
			++src;
			break;

		default:
			*dst++ = *src++;
		}
	}
	*dst = FEND;
}

int kiss_decode( const void *in, size_t len, unsigned char *type, void *out, size_t *decoded_len)
{
	if ( in == NULL || type == NULL || out == NULL )
		return EINVAL;

	unsigned char *src = (unsigned char *)in;
	unsigned char *dst = (unsigned char*)out;
	size_t n = len-3;
	
	if ( len <= 3 || *src++ != FEND )
		return EBADMSG;

	*type = *src++;
	
	while ( n-- > 0 )
	{
		switch ( *src )
		{
		case FEND:
			return EBADMSG;
			break;
		
		case FESC:
			if ( n-- == 0 )
				return EBADMSG;
			
			++src;
			switch ( *src )
			{
			case TFEND:
				*dst++ = FEND;
				break;
			
			case TFESC:
				*dst++ = FESC;
				break;
				
			default:
				return EBADMSG;
			}
			break;
		
		default:
			*dst++ = *src;
		}
		
		++src;
	}
	
	if ( *src != FEND )
		return EBADMSG;

	*decoded_len = dst - (unsigned char*)out;
	
	return 0;
}

void kiss_send( ax_iface *self, void *frame, size_t len)
{
	kiss_iface_attrs *attrs = (kiss_iface_attrs*) self->p;
	size_t buffer_len = kiss_encoded_size( frame, len);
	unsigned char *buffer = (unsigned char *) malloc( buffer_len);
	if ( buffer == NULL )
		return; // FIXME: more robust OOM handling
	
	kiss_encode( frame, len, 0x00U, buffer);
	send( attrs->client_fd, buffer, buffer_len, 0);
	
	free( buffer);
}

void *kiss_client_handler( void* arg)
{
	ax_iface *self = (ax_iface*) arg;
	kiss_iface_attrs *attrs = (kiss_iface_attrs*) self->p;
	
	unsigned char buffer[BUFFER_SIZE];
	size_t bytes_read, decoded_len;
	unsigned char *frame = NULL;
	unsigned char frame_type;
	int result;
	
	while (1) {
		bytes_read = recv(attrs->client_fd, buffer, BUFFER_SIZE - 1, 0);
		if (bytes_read == 0) {
			break;
		}

		frame = malloc( bytes_read);
		if ( frame == NULL )
		{
			printf( "Unable to process frame due to memory allocation error.\n");
			continue;
		}

		result = kiss_decode( buffer, bytes_read, &frame_type, frame, &decoded_len);

		if( result == 0 )
			kiss_recv( self, frame_type, frame, decoded_len);
		else
			printf("Non-KISS frame received and discarded.\n");

		free( frame);
	}
	
	printf("Client disconnected.\n");
	
	// Ask the DSE to shut the interface
	if ( attrs->sw != NULL && attrs->sw->close_iface != NULL )
		attrs->sw->close_iface( attrs->sw, self->id);
	
	while(1) {
		sleep(1);
	}
	return NULL;
}

void kiss_recv( ax_iface *self, unsigned char type, const void *frame, size_t len)
{
	if ( self == NULL || self->p == NULL || frame == NULL )
		return;

	unsigned char *data = (unsigned char *)frame;
	kiss_iface_attrs *attrs = (kiss_iface_attrs*) self->p;

	switch( type & 0x0F )
	{
	case 0x00:
		printf( "New data frame: ");
		kiss_print_frame( frame, len);
		printf( "\n");

		// Switch frame
		if ( attrs->sw != NULL && attrs->sw->switch_frame != NULL )
			attrs->sw->switch_frame( attrs->sw, frame, len);

		break;

	case 0x01:
		if( len >= 1 )
			printf( "Set Tx Delay: %d ms.\n", 10*(uint8_t)*data);
		else
			printf( "Invalid command.");
		break;

	case 0x02:
		if( len >= 1 )
			printf( "Set Persistence: %d.\n", (uint8_t)*data);
		else
			printf( "Invalid command.");
		break;

	case 0x03:
		if( len >= 1 )
			printf( "Set Slot time: %d ms.\n", 10*(uint8_t)*data);
		else
			printf( "Invalid command.");
		break;

	case 0x04:
		if( len >= 1 )
			printf( "Set Tx tail: %d ms.\n", 10*(uint8_t)*data);
		else
			printf( "Invalid command.\n");
		break;

	case 0x05:
		if( len >= 1 )
			printf( "Duplex mode: %s\n", ((uint8_t)*data == 0) ? "Half" : "Full");
		else
			printf( "Invalid command.\n");
		break;

	case 0x0C:
		// TODO: Handle IPOLL command
		printf( "Received an IPOLL command");
		break;

	default:
		printf( "Unsupported KISS command type");
	}
}

void kiss_print_frame( const void *frame, size_t len)
{
	if ( frame == NULL )
		return;

	unsigned char *data = (unsigned char*)frame;

	while ( len-- > 0)
		printf( "%02X ", *data++);
}
