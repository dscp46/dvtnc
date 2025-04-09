#include "dse.h"

/***** Internal functions *****/

// Deallocate the DSE
void dse_free( dse_t *dse);

void dse_add_fib( dse_t *dse, char *callsign, int16_t iface_id, bool is_static);

void dse_del_fib( dse_t *dse, char *callsign);


// Switch a frame
void dse_switch_frame( dse_t *dse, void *frame, size_t len);

// Add a KISS interface to the DSE
bool dse_kiss_add( dse_t *dse, serial_s *serial);

// Close an active interface
void dse_close_iface( dse_t *dse, int16_t ifnum);

/***** Implementation *****/

// Instanciate a Digital Switching Equipment (DSE)
dse *dse_create( void)
{
	dse *sw = malloc( sizeof( dse));
	if ( sw == NULL )
		return NULL;
	
	dse_t *my_dse = malloc( sizeof( dse_t));
	if ( my_dse == NULL )
	{
		dse_free( sw);
		return NULL;
	}
	
	return sw;
}

void dse_free( dse_t *dse)
{
	if ( dse == NULL )
		return;
	
	
	free( dse);
}
