#include "ringbuffer.h"

/** 
 * @brief Create a ring buffer
 * 
 * @param[in] size 	The internal buffer size (size_t)
 *
 * @return 	A pointer to the allocated ring buffer (ringbuffer_t*)
 * @retval NULL		Memory allocation failure..
 */
ringbuffer_t* ringbuffer_alloc( size_t size)
{
	ringbuffer_t *buf = (ringbuffer_t*) malloc(sizeof(ringbuffer_t));
	if ( buf == NULL )
		return NULL;

	buf->buffer = (unsigned char*) malloc( size);
	if ( buf->buffer == NULL )
	{
		free( buf);
		return NULL;
	}

	buf->size = size;
	buf->cur_read = buf->buffer;
	buf->cur_write = buf->buffer;
	
	return buf;
}	

/** 
 * @brief Frees up the resources associated to a ring buffer
 *
 * @param[in] rbuf	The ringbuffer to free (ringbuffer_t*)
 */
void ringbuffer_free( ringbuffer_t *rbuf)
{
	if( rbuf == NULL )
		return;

	if ( rbuf->buffer != NULL )
		free( rbuf->buffer);
	
	rbuf->buffer = NULL;
	free( rbuf);
}

/** 
 * @brief Get how many bytes can be written at the moment
 *
 * @param[in] buf	The ringbuffer we want to check (ringbuffer_t*) 
 * @return 	Number of available bytes to be written, at a given time.(size_t)
 */
size_t ringbuffer_bytes_available( const ringbuffer_t *buf)
{
	// Distance between read and write pointers in a circular manner
	size_t used = (buf->cur_write - buf->cur_read + buf->size) % buf->size;

	return buf->size - used - 1;
}

/**
 * @brief Pull at most n bytes from the ring buffer
 *
 * @param[in] dest	The destination buffer (void*)
 * @param[in] n		Read at most n bytes (size_t)
 * @param[in] buf	The ringbuffer we want to pull bytes from (rungbuffer_t*)
 *
 * @return	The actual number of pulled bytes (size_t)
 */
size_t ringbuffer_pull( void *dest, size_t n, ringbuffer_t *buf)
{
	if ( buf == NULL || dest == NULL )
		return 0;

	size_t used_bytes = (buf->cur_write - buf->cur_read + buf->size) % buf->size;
	size_t n_read = ( n <= used_bytes ) ? n : used_bytes;
	
	unsigned char *dst = (unsigned char*)dest;
	size_t n_remain = n_read;
	
	while ( n_remain-- > 0 )
	{
		*dst++ = *(buf->cur_read++);
		
		if ( buf->cur_read == buf->buffer + buf->size )
			buf->cur_read = buf->buffer;
	}
	
	return n_read;
}

/**
 * @brief Insert at most n bytes into ringbuffer
 * @param[in] src	Source buffer we want to insert data from (const void*)
 * @param[in] n		Number of bytes we want to write to the ringbuffer (size_t)
 * @param[in] buf	The ringbuffer we're writing to (ringbuffer_t*)
 *
 * @return	The number of actually written bytes (size_t)
 */
size_t ringbuffer_push( const void *src, size_t n, ringbuffer_t *buf)
{
	if ( buf == NULL || src == NULL)
		return 0;
	
	size_t avail_bytes = ringbuffer_bytes_available( buf);
	
	size_t n_written = ( n <= avail_bytes ) ? n : avail_bytes;
	size_t n_remain = n_written;
	
	unsigned char *srce = (unsigned char*)src;
	
	while ( n_remain-- > 0 )
	{
		*(buf->cur_write++) = *srce++;
		if ( buf->cur_write == buf->buffer + buf->size )
			buf->cur_write = buf->buffer;
	}
	
	return n_written;
}


