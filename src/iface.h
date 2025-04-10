#ifndef __IFACE_H
#define __IFACE_H

#include <stdint.h>
#include <stdlib.h>
#include <uthash.h>

#define IFACE_INVALID	-1

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
	void (*send_frame)( struct ax_iface *self, void *frame, size_t len);
	void (*close)( struct ax_iface *self);
	
	// UTHash Handle
	UT_hash_handle hh;
	
	// Private data structure
	void *p;
} ax_iface;

#endif	/* __IFACE_H */
