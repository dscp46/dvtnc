#include "./kiss.h"

#include <stdlib.h>

/* Internal definitions */


kiss_server_t* kiss_start_server( char *addr, uint16_t portnum)
{
	kiss_server_t *instance = (kiss_server_t*)malloc( sizeof( kiss_server_t));
	if ( instance == NULL )
		return NULL;
	
	
	return instance;
}
