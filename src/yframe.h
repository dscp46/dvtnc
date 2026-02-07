#ifndef __YFRAME_H
#define __YFRAME_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <utstring.h>

typedef enum {
	UNSYNCED,
	READING,
	UNESC_NEXT,
} yframe_state_t;

// Callback function arguments
typedef struct yframe_cb_args_t {
	void *buf;
	size_t n;
	void *extra_arg;
} yframe_cb_args_t;

typedef void (*yframe_rx_callback)( void*);

typedef struct yframe_ctx {
	// A buffer that will be used to assemble the frame
	UT_string *frame_buffer;
	
	// The current receiver state  
	yframe_state_t state;
	
	// The Maximum Transmission Unit, acts as the buffer's size
	size_t mtu;
	
	// The callback function that will be called once valid frames are received.
	yframe_rx_callback process_frame;
	
	// Extra argument to pass to the reception callback
	void *process_extra_arg;

	void    (*free) ( struct yframe_ctx* ctx);
	void (*receive) ( struct yframe_ctx *ctx, void *buf, size_t n);
} yframe_ctx_t;

yframe_ctx_t* yframe_ctx_create( size_t mtu, yframe_rx_callback process_frame, void *process_extra_arg);
void yframe_encode( const void* buf, size_t n, void **out, size_t *out_size);

#ifdef YFRAME_INTERNALS
void yframe_ctx_free( yframe_ctx_t* ctx);
bool yframe_is_banned_char( unsigned char c);
size_t yframe_encoded_size( const void* buf, size_t n);
void yframe_receive( yframe_ctx_t *ctx, void *_in, size_t n);
#endif

#endif	/* __YFRAME_H */
