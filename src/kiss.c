#include "app.h"
#include "kiss.h"
#include "yframe.h"

#include <arpa/inet.h>
#include <endian.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#define BUFFER_SIZE 1460

// AX.25 frame: 1024b, Start & End markers: 2b, CRC32: 4b, Type: 1b 
#define RX_MTU 1031

#define KISS_FEND	0xC0
#define KISS_FESC	0xDB
#define KISS_TFEND	0xDC
#define KISS_TFESC	0xDD

#define KISS_MASK_CMD	0x0F
#define KISS_MASK_ADDR	0xF0

#define KISS_DATA	0x00
#define KISS_SET_TXDLY	0x01
#define KISS_SET_P	0x02
#define KISS_SET_SLOTTM	0x03
#define KISS_SET_TXTAIL	0x04
#define KISS_SET_FDX	0x05
#define KISS_CMD_SETHW	0x06
#define KISS_ACKMODE	0x0C
#define KISS_IPOLL	0x0E

typedef struct kiss_srv_args_t {
	app_settings_t *settings;
	int client_fd;
} kiss_srv_args_t;

// Decode a KISS payload
void kiss_decode( UT_string *buf, UT_string *out)
{
	if( buf == NULL || out == NULL ) return;
	unsigned char c, found_cmd=0, *in = (unsigned char*) utstring_body( buf);
	size_t len = utstring_len( buf);

	while( len-- > 0 )
	{
		if( found_cmd )
		{
			utstring_bincpy( out, in++, 1);
			found_cmd = 0;
			continue;
		}

		switch( *in)
		{
		case KISS_FEND:
			// Ignore FEND if string is empty
			if( utstring_len(out) != 0 )
				return;
			found_cmd = 1;
			break; // continue in while

		case KISS_FESC:
			break; // continue in while

		case KISS_TFEND:
			++in;
			c = KISS_FEND;
			utstring_bincpy( out, &c, 1);
			break; // continue in while

		case KISS_TFESC:
			++in;
			c = KISS_FESC;
			utstring_bincpy( out, &c, 1);
			break; // continue in while

		default:
			utstring_bincpy( out, in, 1);
		}
		++in;
	}
}

// Encode a raw frame in KISS format.
void kiss_encode( UT_string *buf, UT_string *out, unsigned char cmd)
{
	if( buf == NULL || out == NULL ) return;
	unsigned char c[2], *in = (unsigned char*) utstring_body( buf);
	size_t len = utstring_len( buf);

	c[0] = KISS_FESC;
	c[1] = cmd;
	utstring_bincpy( out, &c, 2);

	while( len-- > 0 )
	{
		switch( *in)
		{
		case KISS_FEND:
			c[1] = KISS_TFEND;
			utstring_bincpy( out, &c, 2);
			break;

		case KISS_FESC:
			c[1] = KISS_TFESC;
			utstring_bincpy( out, &c, 2);
			break;

		default:
			utstring_bincpy( out, in, 1);
		}
		in++;
	}

	utstring_bincpy( out, &c, 1); // Trailing FEND
}

void kiss_process_frame( void* buffer, size_t n, app_settings_t *settings)
{
	unsigned char *ptr = (unsigned char*)buffer, *frame = NULL;
	size_t i = n-1, frame_len;
	UT_string *raw = NULL, *encoded = NULL;
	utstring_new( raw);
	utstring_new( encoded);
	
	//uint16_t acknum;
	uint8_t type = (uint8_t) *ptr++;
	uint32_t fcs;

	switch( type & 0x0F )
	{
	case KISS_ACKMODE:
		printf( "[ACKMODE] ");
		//acknum = ntohs( *ptr);
		frame = ptr + 2;
		i -= 2;
		// TODO: add send queue tracking to send ACK once the packet has been aired
		// fall through

	case KISS_DATA:
		frame_len = i;
		if( frame == NULL )
			frame = ptr;

		printf("New data frame: ");
		while ( i-- > 0 )
			printf( "%02X ", *ptr++);
		printf("\n");

		// Decode KISS payload
		utstring_bincpy( encoded, frame, frame_len);
		kiss_decode( encoded, raw);

		// Clear the buffer for later use (encoded YFrame)
		utstring_clear( encoded);

		// Compute and store checksum
		fcs = crc32(  0L, Z_NULL, 0);
		fcs = crc32( fcs, frame, frame_len);
		fcs = htole32( fcs);
		utstring_bincpy( raw, &fcs, 4);

		// Encapsulate in a YFRAME
		yframe_encode( utstring_body( raw), utstring_len( raw), encoded);

		// Send packet
		serial_send( settings->serial, utstring_body( encoded), utstring_len( encoded));
		break;

	case KISS_SET_TXDLY:
		if( n == 2 )
			printf( "Set Tx Delay: %d ms.\n", 10*(uint8_t)*ptr);
		else
			printf( "Invalid command.");
		break;

	case KISS_SET_P:
		if( n == 2 )
			printf( "Set Persistence: %d.\n", *ptr);
		else
			printf( "Invalid command.");
		break;

	case KISS_SET_SLOTTM:
		if( n == 2 )
			printf( "Set Slot time: %d ms.\n", 10*(uint8_t)*ptr);
		else
			printf( "Invalid command.");
		break;

	case KISS_SET_TXTAIL:
		if( n == 2 )
			printf( "Set Tx tail: %d ms.\n", 10*(uint8_t)*ptr);
		else
			printf( "Invalid command.\n");
		break;

	case KISS_SET_FDX:
		if( n == 2 )
			printf( "Duplex mode: %s\n", ((uint8_t)*ptr == 0) ? "Half" : "Full");
		else
			printf( "Invalid command.\n");
		break;

	case KISS_CMD_SETHW:
		printf("Hardware specific command: ");

		utstring_bincpy( encoded, ptr, i);
		kiss_decode( encoded, raw);
		ptr = (unsigned char *) utstring_body( raw);
		i = utstring_len( raw);

		while( i-- > 0)
			printf( "%02X ", *ptr++);
		printf("\n");

		// TODO: Process Command
		break;

	case KISS_IPOLL:
		printf( "Received an IPOLL command");
		break;

	default:
		printf( "Unsupported command type");
	}

	utstring_free( raw);
	utstring_free( encoded);
}

void *kiss_handle_client( void *arg)
{
	kiss_srv_args_t *args = (kiss_srv_args_t*) arg;
	int client_fd = args->client_fd;
	app_settings_t *settings = args->settings;
    free(args);
    
    unsigned char buffer[BUFFER_SIZE];
    printf("KISS Client connected.\n");

    while (1) {
        size_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            break;
        }
        
        if( buffer[0] == 0xC0 && buffer[bytes_read-1] == 0xC0 )
        	kiss_process_frame( buffer+1, bytes_read-2, settings);
        else
        	printf(" Non-KISS frame received and discarded.");
    }

    printf("Client disconnected.\n");
    close(client_fd);
    return NULL;
}

void kiss_dispatch_processed_frame( void *arg)
{
	
	yframe_cb_args_t *args = (yframe_cb_args_t *)arg;
	app_settings_t *settings = (app_settings_t*)args->extra_arg;
	unsigned char* data = (unsigned char*)args->buf;
	size_t len = args->n;
	uint32_t fcs, rx_fcs;
	UT_string *raw = NULL, *encoded = NULL;

	// Runt frames are less than the short AX.25 frame and its FCS
	if( len < 20 )
	{
		// TODO: Runt frame processing
		return;
	}

	memcpy( &rx_fcs, data+(len-4), 4);
	rx_fcs = le32toh( rx_fcs);

	fcs = crc32(  0L, Z_NULL, 0);
	fcs = crc32( fcs, data, len-4);

	if( fcs == rx_fcs )
	{
		utstring_new( raw);
		utstring_new( encoded);

		utstring_bincpy( raw, data, len-4);
		kiss_encode( raw, encoded, KISS_DATA);

		// FIXME: Pass down the frame to the AX.25 DSE layer for switching
		send( settings->client_fd, utstring_body( encoded), utstring_len( encoded), 0);

		utstring_free( raw);
		utstring_free( encoded);
	}
	else
	{
		fprintf( stderr, "Received corrupted frame (FCS: %08x, expected %08x).\n", fcs, rx_fcs);
	}
}

void kiss_yframe_pass( void *ctx, void *buf, size_t n)
{
	app_settings_t *settings = (app_settings_t*)ctx;
	settings->yframe_rx_ctx->receive( settings->yframe_rx_ctx, buf, n);
}

void *kiss_server( void *arg)
{
	if ( arg == NULL )
		return NULL;

	app_settings_t *settings = (app_settings_t*) arg;
	
	int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(settings->kiss_port);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("KISS Server listening on port %d\n", settings->kiss_port);
   
    settings->yframe_rx_ctx = yframe_ctx_create( RX_MTU, kiss_dispatch_processed_frame, settings);

    // Set up a receiver function for the current interface
    serial_update_rx_processor( settings->serial, kiss_yframe_pass, settings);


    while (1) {
    	kiss_srv_args_t *args = malloc(sizeof(kiss_srv_args_t));
        if (!args) {
            perror("Memory allocation failed");
            continue;
        }
        
	args->settings = settings;
	args->client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	
        if (args->client_fd == -1) {
            perror("Accept failed");
            free(args);
            continue;
        }
        
        // FIXME: Insert the client FD into a hashmap
        settings->client_fd = args->client_fd;
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, kiss_handle_client, args) != 0) {
            perror("Thread creation failed");
            free(args);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}

void kiss_start_server( app_settings_t *settings)
{
	if ( settings == NULL )
		return;

	pthread_create( &settings->kiss_srv_thread, NULL, kiss_server, settings);
}

void kiss_stop_server( app_settings_t *settings)
{
	if ( settings == NULL )
		return;

	pthread_cancel( settings->kiss_srv_thread);
}
