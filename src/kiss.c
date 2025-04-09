#include "app.h"
#include "kiss.h"
#include "yframe.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1460

// AX.25 frame: 1024b, Start & End markers: 2b, CRC32: 4b, Type: 1b 
#define RX_MTU 1031

typedef struct kiss_srv_args_t {
	app_settings_t *settings;
	int client_fd;
} kiss_srv_args_t;

void kiss_process_frame( void* buffer, size_t n, app_settings_t *settings)
{
	unsigned char *ptr = (unsigned char*)buffer;
	size_t i = n-1;
	
	uint8_t type = (uint8_t) *ptr++;
	
	switch( type & 0x0F )
	{
	case 0x00:
		printf("New data frame: ");
		while ( i-- > 0 )
			printf( "%02X ", *ptr++);
		printf("\n");
		
		// TODO: Encapsulate in a YFRAME
		void *encoded = NULL;
    	size_t encoded_size = 0;
    
    	yframe_encode(buffer, n, &encoded, &encoded_size);
		serial_send( settings->serial, encoded, encoded_size);
		
		if ( encoded != NULL )
			free( encoded);

		break;
		
	case 0x01:
		if( n == 2 )
			printf( "Set Tx Delay: %d ms.\n", 10*(uint8_t)*ptr);
		else
			printf( "Invalid command.");
		break;
	
	case 0x02:
		if( n == 2 )
			printf( "Set Persistence: %d.\n", *ptr);
		else
			printf( "Invalid command.");
		break;
	
	case 0x03:
		if( n == 2 )
			printf( "Set Slot time: %d ms.\n", 10*(uint8_t)*ptr);
		else
			printf( "Invalid command.");
		break;

	case 0x04:
		if( n == 2 )
			printf( "Set Tx tail: %d ms.\n", 10*(uint8_t)*ptr);
		else
			printf( "Invalid command.\n");
		break;
	
	case 0x05:
		if( n == 2 )
			printf( "Duplex mode: %s\n", ((uint8_t)*ptr == 0) ? "Half" : "Full");
		else
			printf( "Invalid command.\n");
		break;
	
	case 0x06:
		printf("Hardware specific command: ");
		while ( i-- > 0 )
			printf( "%02X ", *ptr++);
		printf("\n");
		break;
		
	case 0x0C:
		printf( "Received an IPOLL command");
		break;
		
	default:
		printf( "Unsupported command type");
	}

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
	
	switch( data[0] )
	{
	case 0x00:
		// Data frame.
		// FIXME: Pass down the frame to the AX.25 DSE layer for switching
		unsigned char *frame = malloc( args->n+2);
		if ( frame == NULL )
		{
			fprintf( stderr, "kiss_displatch_processed_frame: memory alloc failed.\n");
			break;
		}
		frame[0] = 0xC0;
		frame[args->n+1] = 0xC0;
		memcpy( frame+1, data, args->n);
		send( settings->client_fd, frame, args->n+2, 0);
		free( frame);	
		break;
	default:
		fprintf( stderr, "Received a data frame with unknown type: %x", data[0]);
	}
}

void kiss_yframe_pass( void *ctx, void *buf, size_t n)
{
	app_settings_t *settings = (app_settings_t*)ctx;
	yframe_receive( settings->yframe_rx_ctx, buf, n);
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
