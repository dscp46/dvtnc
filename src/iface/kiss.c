
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

// KISS protocol magic values
#define FEND	0xC0U
#define FESC	0xDBU
#define TFEND	0xDCU
#define TFESC	0xDDU

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

size_t kiss_encoded_size( const void* frame, size_t len);
void kiss_encode( const void *in, size_t len, unsigned char type, void *out);
int kiss_decode( const void *in, size_t len, unsigned char *type, void *out);

void kiss_send( ax_iface *self, void *frame, size_t len);
void *kiss_client_handler( void* arg);

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

int kiss_decode( const void *in, size_t len, unsigned char *type, void *out)
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
	
	while (1) {
		size_t bytes_read = recv(attrs->client_fd, buffer, BUFFER_SIZE - 1, 0);
		if (bytes_read == 0) {
			break;
		}

		if( buffer[0] == 0xC0 && buffer[bytes_read-1] == 0xC0 )
			;// TODO: kiss_process_frame( buffer+1, bytes_read-2, settings);
		else
			printf(" Non-KISS frame received and discarded.");
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

