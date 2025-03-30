#include <string.h>

#include "yframe.h"

#define YFRAME_START	0xE1
#define YFRAME_END		0xE0
#define YFRAME_ESC		0x3D
#define YFRAME_OFFSET	64

const unsigned char YFRAME_BANNED_CHARS = {
	0x00, 0x11, 0x13, 0x1A, YFRAME_ESC, 0x84, YFRAME_END, YFRAME_START, 0xFD, 0xFE, 0xFF
};

const int8_t YFRAME_BANNED_CHARS_SZ = 11;

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
	ctx->status = UNSYNCED;
	
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
	int8_t n = YFRAME_BANNED_CHARS_SZ-1;
	
	do
	{
		if ( c == YFRAME_BANNED_CHARS[n] )
			return true;
	}
	while ( n-- > 0 )
	
	return false;
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

void yframe_encode( void* buf, size_t n, void **out, size_t *out_size)
{
	*out_size = yframe_encoded_size( buf, n);
	if ( *out_size == 0 )
		return;
	
	*out = malloc( *out_size);
	if ( *out == NULL )
	{
		*out_size = 0;
		return;
	}
	
	unsigned char *in = (unsigned char *) buf;
	unsigned char *res = (unsigned char *) *out;
	int16_t	c;
	
	*res++ = YFRAME_START;
	
	while ( n-- > 0 )
	{
		if( yframe_is_banned_char( *buff++) )
		{
			*res++ = YFRAME_ESC;
			*res++ = YFRAME_OFFSET + *in++
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

	unsigned char *in = (unsigned char *)in;
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
				// TODO: if ( is_valid_checksum( ctx->buffer, ctx->cur_buf_size) )
				if ( true )
				{
					yframe_cb_args_t args;
					args->buf = ctx->buffer;
					args->n = ctx->cur_buf_size;
					(*process_frame)( &args);
				}
				
				ctx->state = UNSYNCED;
				break;
			}
						
			if ( c == YFRAME_ESC )
			{
				ctx->state = UNESC_NEXT;
				break;
			}
			
			ctx->buffer[ctx->cur_buf_size++] = c;
			if ( ctx->cur_buf_size >= ctx->mtu )
				ctx->state = UNSYNCED;

			break;
			
		case YFRAME_ESC:
			ctx->buffer[ctx->cur_buf_size++] = c - YFRAME_OFFSET;
			if ( ctx->cur_buf_size >= ctx->mtu )
				ctx->state = UNSYNCED;
			
			ctx->state = READING;
			break;
		
		default:
			ctx->state = UNSYNCED;
		}
	}
}

