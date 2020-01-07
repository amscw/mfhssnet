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
struct mfhss_pkt_;

// Main device structure
struct mfhss_priv_ {
	struct net_device *dev;
	struct net_device_stats *stats;
	void __iomem *io_base;
	int status;
	struct mfhss_pkt_ *pool;			// Pointer to last item in list
	struct mfhss_pkt_ *rx_queue;		// List of incoming packets 
	int rx_int_en;
	int tx_pkt_len;
	u8 *tx_pkt_data;
	struct sk_buff *skb;
	spinlock_t lock;
};

//-------------------------------------------------------------------------------------------------
// Varibles
//-------------------------------------------------------------------------------------------------
extern struct net_device *mfhss_dev;
#endif // _MFHSSNET_H
