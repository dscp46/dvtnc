#ifndef __DRIVER_H
#define __DRIVER_H

#include <stdint.h>

// Callback function for received frames
typedef size_t (*driver_rx_callback_t)( void *buf, size_t buf_sz);

typedef struct tx_model {
	//  Device basics: Open and close device
	int (*open)( const char* path, void *config);
	void (*close)( int fd);
	
	//  User plane: the interface used to transfer data
	void (*data_tx)( const void *buf, size_t n);
	void (*set_data_rx_callback)( driver_rx_callback_t cb);
	
	//  Control plane: the interface used to handle the rig
	void (*ctl_chan_tx)( const void *buf, size_t n);
	void (*set_ctl_chan_rx_callback)( const void *buf, size_t n);
	
	void *self;
} tx_model;

typedef struct device {
	void (*qsy)( uint64_t freq);
	void (*save_cfg)( void *cfg);
	void (*configure)( void);
	void (*restore_cfg)( const void *cfg);
} device;

#endif	/* __DRIVER_H */
