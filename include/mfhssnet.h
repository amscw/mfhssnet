#ifndef _MFHSSNET_H
#define _MFHSSNET_H

#include "common.h"

//-------------------------------------------------------------------------------------------------
// MACRO
//-------------------------------------------------------------------------------------------------
// These are the flags in the statusword
#define DUMMY_NETDEV_RX_INTR 	0x0001
#define DUMMY_NETDEV_TX_INTR 	0x0002

// Default timeout period
#define DUMMY_NETDEV_TIMEOUT 	5 // In jiffies

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
// Incompletes
struct net_device;
struct net_device_stats;
struct sk_buff;
struct kset;

// Payload data packet structure
struct mfhss_pkt_
{
	struct net_device *dev;
	int datalen;
	u8 *data; //ETH_DATA_LEN
	struct list_head list;
};

// Main device structure
struct mfhss_priv_ {
	// platform data
	struct resource resource;
	void __iomem *io_base;
	char *src_addr;
	char *dst_addr;
	dma_addr_t src_handle;
	dma_addr_t dst_handle;
	int irq_rx;
	int irq_tx;

	// network data
	struct net_device *dev;
	struct net_device_stats *stats;
	struct sk_buff *skb;
	int tx_pkt_len;
	u8 *tx_pkt_data;

	// locker
	spinlock_t lock;
	
	// sysfs data
	struct kset *static_regs;
	struct kset *dynamic_regs;
	
	// not use (compat) 
	int status;
	int rx_int_en;
};


//-------------------------------------------------------------------------------------------------
// Varibles
//-------------------------------------------------------------------------------------------------
extern struct net_device *mfhss_dev;
#endif // _MFHSSNET_H
