#ifndef __DSE_H
#define __DSE_H

#include "iface.h"
#include "serial.h"

#include <stdbool.h>
#include <stdint.h>
#include <uthash.h>

/*** Entries for the FIB and RIB ***/

typedef struct dse_entry {
	// AX.25 address in integer form
	uint64_t id;
	
	// Destination Interface
	int16_t iface;
	
	bool static_entry;
	
	// Hashtable handle
	UT_hash_handle hh;
	
} dse_entry;


typedef struct dse_t {
	// Forwarding Information Base
	dse_entry *fib;
	
	// Routing Information Base
	dse_entry *rib;
	
	ax_iface *if_list;
} dse_t;

typedef struct dse {
	dse_t *sw;
	
	void (*free)( struct dse *self);
	void (*add_fib)( struct dse *self, char *callsign, int16_t iface_id, bool is_static);
	void (*del_fib)( struct dse *self, char *callsign);
	void (*print_fib)( struct dse *self);
	
	void (*add_rib)( struct dse *self, char *callsign, int16_t iface_id, bool is_static);
	void (*del_rib)( struct dse *self, char *callsign);
	void (*print_rib)( struct dse *self);
	
	void (*switch_frame)( struct dse *self, void *frame, size_t len);
	
	bool (*dse_kiss_add)( struct dse *self, serial_t *serial);
	void (*close_iface)( struct dse *self, int16_t ifnum);
} dse;

// Instanciate a Digital Switching Equipment (DSE)
dse *dse_create( void);

#endif	/* __DSE_H */
