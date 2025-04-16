#ifndef __IFACE_KISS_H
#define __IFACE_KISS_H

#include "../iface.h"

#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>


ax_iface *kiss_ifup( int client_fd, struct sockaddr* client_addr, size_t client_addr_sz);
void kiss_ifdown( ax_iface *iface);

#endif	/* __IFACE_KISS_H */
