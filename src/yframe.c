#include "yframe.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define YFRAME_START	0xE1
#define YFRAME_END	0xE0
#define YFRAME_ESC	0x3D
#define YFRAME_OFFSET	64

static uint8_t BANNED_CHARMAP[32] = {0};
static bool charmap_inited = false;

// Initialize the banned characters map. This is done to drop from O(n) from O(1) on banned chars lookup
void __yframe_init_banned_charmap() {
	const unsigned char YFRAME_BANNED_CHARS[] = {
		0x00, 0x11, 0x13, 0x1A, YFRAME_ESC, 0x84, YFRAME_END, YFRAME_START, 0xFD, 0xFE, 0xFF
	};
	
	for( size_t i=0; i<sizeof(YFRAME_BANNED_CHARS); ++i )
		BANNED_CHARMAP[ YFRAME_BANNED_CHARS[i] / 8 ] |= ( 1 << (YFRAME_BANNED_CHARS[i] % 8));
}

void yframe_print( void* _args)
{
	if ( _args == NULL )
		return;
	
	yframe_cb_args_t *args = (yframe_cb_args_t*) _args;
	
	if ( args->buf == NULL || args->n == 0 )
		return;
		
	printf( "New frame:\n" );
	size_t n = args->n;
	unsigned char *buf = (unsigned char*) args->buf;
	
	while ( n-- > 0 )
	{
		printf( "%02X ", (unsigned char) *buf++ );
	}
	
	printf( "\n");
}

yframe_ctx_t* yframe_ctx_create( size_t mtu, yframe_rx_callback process_frame)
{
	yframe_ctx_t* ctx = (yframe_ctx_t*) malloc( sizeof(yframe_ctx_t));
	if ( ctx == NULL )
		return NULL;

	ctx->mtu = mtu;
	ctx->frame_buffer = (char*) malloc( mtu);
	if ( ctx->frame_buffer == NULL )
	{
		yframe_ctx_free( ctx);
		return NULL;
	}
	
	ctx->cur_buf_size = 0;
	ctx->state = UNSYNCED;
	
	ctx->process_frame = ( process_frame != NULL ) ? process_frame : yframe_print;
	
	return ctx;
}

void yframe_ctx_free( yframe_ctx_t* ctx)
{
	if ( ctx == NULL )
		return;
	
	if ( ctx->frame_buffer != NULL )
	{
		free( ctx->frame_buffer);
	}
	
	free( ctx);
}

bool yframe_is_banned_char( unsigned char c)
{
	if ( !charmap_inited )
	{
		__yframe_init_banned_charmap();
		charmap_inited = true;
	}
	
	return ( BANNED_CHARMAP[ c/8 ] & (1 << ( c%8 )) ) != 0;
}

size_t yframe_encoded_size( const void* buf, size_t n)
{
	size_t outsize = 2; /* FIXME: 4 once we've inserted the frame checksum */
	if ( buf == NULL )
		return 0;
	
	unsigned char *buff = (unsigned char*) buf;
	while ( n-- > 0 )
	{
		// Extra incrementation for banned chars
		if ( yframe_is_banned_char( *buff++) )
			++outsize;
		
		++outsize;
	}
	return outsize;
}

// Allocate a buffer and encode the frame.
// The end user is responsible for freeing the out buffer. 
void yframe_encode( const void* src, size_t n, void **out, size_t *out_size)
{
	*out_size = yframe_encoded_size( src, n);
	if ( *out_size == 0 )
		return;
	
	*out = malloc( *out_size);
	if ( *out == NULL )
	{
		*out_size = 0;
		return;
	}
	
	const unsigned char *in = (const unsigned char *) src;
	unsigned char *res = (unsigned char *) *out;
	//int16_t	c;
	
	*res++ = YFRAME_START;
	
	while ( n-- > 0 )
	{
		if( yframe_is_banned_char( *in) )
		{
			*res++ = YFRAME_ESC;
			*res++ = YFRAME_OFFSET + *in++;
		}
		else
			*res++ = *in++;
	}
	
	*res++ = YFRAME_END;
}

void yframe_receive( yframe_ctx_t *ctx, void *_in, size_t n)
{
	if ( ctx == NULL || _in == NULL )
		return;

	unsigned char *in = (unsigned char *)_in;
	unsigned char c;
	
	while ( n-- > 0 )
	{
		c = *in++;
		
		switch( ctx->state)
		{
		case UNSYNCED:
			if ( c == YFRAME_START )
				ctx->state = READING;
			break;

		case READING:
			if ( c == YFRAME_END )
			{
				// TODO: if ( is_valid_checksum( ctx->frame_buffer, ctx->cur_buf_size) )
				if ( true )
				{
					yframe_cb_args_t args;
					args.buf = ctx->frame_buffer;
					args.n = ctx->cur_buf_size;
					(*ctx->process_frame)( &args);
				}
				
				ctx->state = UNSYNCED;
				ctx->cur_buf_size = 0;
				break;
			}

			if ( c == YFRAME_ESC )
			{
				ctx->state = UNESC_NEXT;
				break;
			}

			if ( ctx->cur_buf_size >= ctx->mtu )
			{
				ctx->state = UNSYNCED;
				ctx->cur_buf_size = 0;
			}

			ctx->frame_buffer[ctx->cur_buf_size++] = c;
			break;
			
		case UNESC_NEXT:
			ctx->frame_buffer[ctx->cur_buf_size++] = c - YFRAME_OFFSET;
			if ( ctx->cur_buf_size >= ctx->mtu )
			{
				ctx->state = UNSYNCED;
				ctx->cur_buf_size = 0;
			}
			ctx->state = READING;
			break;
		
		default:
			ctx->state = UNSYNCED;
			ctx->cur_buf_size = 0;
		}
	}
}

