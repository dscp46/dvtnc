#ifndef __DSE_H
#define __DSE_H

#include "serial.h"
//#include "uthash.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum if_type {
	IF_LOOPBACK,		// Loopback interface
	IF_AXIP,		// AX.25 over IP
	IF_DV_PLUS,		// DPlus Dongle/Repeater Link
	IF_DV_EXTRA,		// DExtra Repeater Link
	IF_KISS,		// KISS over TCP
	IF_AGW,			// AGW over TCP
	IF_SERIAL,		// Serial radio
	IF_NULLMODEM,		// Back-to-back KISS serial line
	IF_PSEUDOTTY		// KISS Pseudo-TTY (to plug into Linux's network stack
} if_type;


/*** Entries for the FIB and RIB ***/

typedef struct dse_entry {
	// AX.25 address in integer form
	uint64_t id;
	
	// Destination Interface
	int16_t iface;
	
	bool static_entry;
	
	// UTHash
	//UT_hash_handle hh;
	
} dse_entry;

/*** AX.25 Interface ***/

#define IFACE_INVALID	-1

typedef struct ax_iface {
	// Interface index
	int16_t id;
	
	// Interface name
	char *name;
	
	// Remote callsign/identity
	char *remote;
	
	// Interface type
	if_type type;
	
	// Function pointers
	void (*send_frame)( void*, size_t);
	void (*close)( void);
	
	// UTHash Handle
	//UT_hash_handle hh;
	
	// Private data structure
	void *p;
} ax_iface;

typedef struct dse_t {
	// Forwarding Information Base
	dse_entry *fib;
	
	// Routing Information Base
	dse_entry *rib;
	
	ax_iface *if_list;
} dse_t;

typedef struct dse {
	dse_t *sw;
	
	void (*free)( dse *self);
	void (*add_fib)( dse *self, char *callsign, int16_t iface_id, bool is_static);
	void (*del_fib)( dse *self, char *callsign);
	void (*print_fib)( dse *self);
	
	void (*add_rib)( dse *self, char *callsign, int16_t iface_id, bool is_static);
	void (*del_rib)( dse *self, char *callsign);
	void (*print_rib)(dse *self);
	
	void (*switch_frame)( dse *self, void *frame, size_t len);
	
	bool (*dse_kiss_add)( dse *self, serial_s *serial);
	void (*close_iface)( dse *self, int16_t ifnum);
} dse;

// Instanciate a Digital Switching Equipment (DSE)
dse *dse_create( void);

#endif	/* __DSE_H */
