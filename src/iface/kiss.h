#ifndef __IFACE_KISS_H
#define __IFACE_KISS_H

#include <stdint.h>

typedef struct kiss_server_t {
	
} kiss_server_t;

kiss_server_t* kiss_start_server( char *addr, uint16_t portnum);

#endif	/* __IFACE_KISS_H */
