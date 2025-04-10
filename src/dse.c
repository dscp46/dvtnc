#include "dse.h"

#include <stdlib.h>

/***** Internal functions *****/

// Deallocate the DSE
void dse_free( struct dse *self);

void dse_add_fib( struct dse *self, char *callsign, int16_t iface_id, bool is_static);

void dse_del_fib( struct dse *self, char *callsign);

// Switch a frame
void dse_switch_frame( struct dse *self, void *frame, size_t len);

// Add a KISS interface to the DSE
bool dse_kiss_add( struct dse *self, serial_t *serial);

// Close an active interface
void dse_close_iface( struct dse *self, int16_t ifnum);

/***** Implementation *****/

// Instanciate a Digital Switching Equipment (DSE)
dse *dse_create( void)
{
	struct dse *instance = (struct dse *) malloc( sizeof( struct dse));
	if ( instance == NULL )
		return NULL;
	
	// Define function pointers
	instance->free = dse_free;
	
	dse_t *sw = malloc( sizeof( dse_t));
	if ( sw == NULL )
	{
		instance->free( instance);
		return NULL;
	}

	instance->sw = sw;	
	return instance;
}

void dse_free( struct dse *self)
{	
	if ( self == NULL )
		return;

	if ( self->sw != NULL )
		free( self->sw);
	
	free( self);
}
