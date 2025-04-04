#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include <pthread.h>
#include <stdint.h>

typedef struct ringbuffer_t {
	unsigned char *buffer;
	size_t size;
	unsigned char *cur_read;
	unsigned char *cur_write;
	pthread_mutex_t mutex;
} ringbuffer_t;

ringbuffer_t* ringbuffer_alloc( size_t size);
void ringbuffer_free( ringbuffer_t *rbuf);

size_t ringbuffer_pull( void *dest, size_t n, ringbuffer_t *buf);
size_t ringbuffer_push( const void *src, size_t n, ringbuffer_t *buf);
size_t ringbuffer_bytes_available( const ringbuffer_t *buf);
size_t ringbuffer_bytes_used( const ringbuffer_t *buf);

#endif	/* __RINGBUFFER_H */
