
#include "./kiss.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*** Internal definitions ***/

// TCP Buffer size
#define BUFFER_SIZE 1460

// AX.25 frame: 1024b, Start & End markers: 2b, CRC32: 4b, Type: 1b 
#define RX_MTU 1031

typedef struct kiss_iface_attrs {
	int client_fd;
	struct sockaddr* client_addr;
	size_t client_addr_sz;
} kiss_iface_attrs;

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
	iface->p = attrs;
	
	iface->send_frame = kiss_send;
	
	// TODO Start handler thread
		
	return iface;
}

void kiss_ifdown( ax_iface *iface)
{
	if ( iface == NULL )
		return;
	
	// TODO Stop handler thread
	
	kiss_iface_attrs* attrs = (kiss_iface_attrs*) iface->p;
	if ( attrs != NULL )
	{
		fclose( attrs->client_fd);
		
		if ( attrs->client_addr != NULL )
			free( attrs->client_addr);
		
		free( attrs);
	}
	
	free( iface);
}

void kiss_send( ax_iface *self, void *frame, size_t len)
{
}

void *kiss_client_handler( void* arg)
{
	ax_iface *self = (ax_iface*) arg;
	kiss_iface_attrs *attrs = (kiss_iface_attrs*) self->p;
	
	unsigned char buffer[BUFFER_SIZE];
	
	while (1) {
		size_t bytes_read = recv(attrs->client_fd, buffer, BUFFER_SIZE - 1, 0);
		if (bytes_read <= 0) {
			break;
		}

		if( buffer[0] == 0xC0 && buffer[bytes_read-1] == 0xC0 )
			;// TODO: kiss_process_frame( buffer+1, bytes_read-2, settings);
		else
			printf(" Non-KISS frame received and discarded.");
	}
	
	printf("Client disconnected.\n");
	self->ifdown( self);
	while(1) {
		sleep(1);
	}
	return NULL;
}

