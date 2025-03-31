#ifndef __YFRAME_H
#define __YFRAME_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum {
	UNSYNCED,
	READING,
	UNESC_NEXT,
} yframe_state_t;

typedef struct yframe_cb_args_t {
	void *buf;
	size_t n;
} yframe_cb_args_t;

typedef void (*yframe_rx_callback)( void*);

typedef struct yframe_ctx_t {
	// A buffer that will be used to assemble the frame
	char *frame_buffer;
	
	// The current receiver state  
	yframe_state_t state;
	
	// Where we currently are in the buffer
	size_t cur_buf_size;
	
	// The Maximum Transmission Unit, acts as the buffer's size
	size_t mtu;
	
	yframe_rx_callback process_frame;
} yframe_ctx_t;


yframe_ctx_t* yframe_ctx_create( size_t mtu, yframe_rx_callback process_frame);

void yframe_ctx_free( yframe_ctx_t* ctx);

bool yframe_is_banned_char( unsigned char c);

size_t yframe_encoded_size( const void* buf, size_t n);

void yframe_encode( void* buf, size_t n, void **out, size_t *out_size);

void yframe_receive( yframe_ctx_t *ctx, void *_in, size_t n);

#endif	/* __YFRAME_H */
