#define YFRAME_INTERNALS
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
		0x00, 0x11, 0x13, 0x1A, 0x24, YFRAME_ESC, 0xCB, YFRAME_END, YFRAME_START, 0xFD, 0xFE, 0xFF
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

yframe_ctx_t* yframe_ctx_create( size_t mtu, yframe_rx_callback process_frame, void *process_extra_arg)
{
	yframe_ctx_t* ctx = (yframe_ctx_t*) malloc( sizeof(yframe_ctx_t));
	if ( ctx == NULL )
		return NULL;

	utstring_new( ctx->frame_buffer);
	
	ctx->mtu = mtu;
	ctx->state = UNSYNCED;
	ctx->process_frame = ( process_frame != NULL ) ? process_frame : yframe_print;
	ctx->process_extra_arg = process_extra_arg;

	ctx->free = yframe_ctx_free;
	ctx->receive = yframe_receive;
	
	return ctx;
}

void yframe_ctx_free( yframe_ctx_t* ctx)
{
	if ( ctx == NULL )
		return;
	
	if ( ctx->frame_buffer != NULL )
	{
		utstring_free( ctx->frame_buffer);
		ctx->frame_buffer = NULL;
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

// Allocate a buffer and encode the frame.
void yframe_encode( const void* src, size_t n, UT_string *out)
{
	if( src == NULL	|| out == NULL )
		return;

	const unsigned char *in = (const unsigned char *) src;
	unsigned char c[2];

	c[0] = YFRAME_START;
	utstring_bincpy( out, &c, 1);

	while ( n-- > 0 )
	{
		if( yframe_is_banned_char( *in) )
		{
			c[0] = YFRAME_ESC;
			c[1] = YFRAME_OFFSET + *in++;
			utstring_bincpy( out, &c, 2);
		}
		else
		{
			c[0] = *in++;
			utstring_bincpy( out, &c, 1);
		}
	}
	
	c[0] = YFRAME_END;
	utstring_bincpy( out, &c, 1);
}

void yframe_receive( yframe_ctx_t *ctx, void *buf, size_t n)
{
	if ( ctx == NULL || buf == NULL )
		return;

	unsigned char *in = (unsigned char *)buf;
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
				if ( true )
				{
					yframe_cb_args_t args;
					args.buf = utstring_body( ctx->frame_buffer);
					args.n = utstring_len( ctx->frame_buffer);
					args.extra_arg = ctx->process_extra_arg;
					(*ctx->process_frame)( &args);
				}
				
				ctx->state = UNSYNCED;
				utstring_clear( ctx->frame_buffer);
				break;
			}

			if ( c == YFRAME_ESC )
			{
				ctx->state = UNESC_NEXT;
				break;
			}

			if ( utstring_len( ctx->frame_buffer) >= ctx->mtu )
			{
				ctx->state = UNSYNCED;
				utstring_clear( ctx->frame_buffer);
			}

			utstring_bincpy( ctx->frame_buffer, &c, 1);
			break;
			
		case UNESC_NEXT:
			if ( utstring_len( ctx->frame_buffer) >= ctx->mtu )
			{
				ctx->state = UNSYNCED;
				utstring_clear( ctx->frame_buffer);
			}

			c -= YFRAME_OFFSET;
			utstring_bincpy( ctx->frame_buffer, &c, 1);
			ctx->state = READING;
			break;
		
		default:
			ctx->state = UNSYNCED;
			utstring_clear( ctx->frame_buffer);
		}
	}
}

