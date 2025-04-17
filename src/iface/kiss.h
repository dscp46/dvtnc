#ifndef __IFACE_KISS_H
#define __IFACE_KISS_H

#include "../iface.h"

#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>

// KISS protocol magic values
#define FEND	0xC0U
#define FESC	0xDBU
#define TFEND	0xDCU
#define TFESC	0xDDU

ax_iface *kiss_ifup( int client_fd, struct sockaddr* client_addr, size_t client_addr_sz);
void kiss_ifdown( ax_iface *iface);

size_t kiss_encoded_size( const void* frame, size_t len);
void kiss_encode( const void *in, size_t len, unsigned char type, void *out);
int kiss_decode( const void *in, size_t len, unsigned char *type, void *out, size_t *decoded_len);

#endif	/* __IFACE_KISS_H */
