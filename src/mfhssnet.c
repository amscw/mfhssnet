#include "mfhssnet.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/in.h>
#include <linux/netdevice.h>	// struct device and other headers
#include <linux/etherdevice.h>	// eth_type_trans
#include <linux/ip.h>			// struct iphdr
#include <linux/tcp.h>			// struct tcphdr
#include <linux/skbuff.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("amscw");			// https://github.com/amscw

//-------------------------------------------------------------------------------------------------
// MACRO
//-------------------------------------------------------------------------------------------------
#define POOL_SIZE	8

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
struct mfhss_pkt_
{
	struct mfhss_pkt_ *next;
	struct net_device *dev;
	int datalen;
	u8 data[ETH_DATA_LEN];
};

//-------------------------------------------------------------------------------------------------
// Prototypes
//-------------------------------------------------------------------------------------------------
static int mfhss_open(struct net_device *dev); 
static int mfhss_close(struct net_device *dev);
static int mfhss_config(struct net_device *dev, struct ifmap *map);
// int dumTxPkt(struct sk_buff *pSkB, struct net_device *pDev);
static int mfhss_ioctl(struct net_device *dev, struct ifreq *req, int cmd);
static int mfhss_change_mtu(struct net_device *dev, int new_mtu);
static void mfhss_tx_timeout(struct net_device *dev);
static struct net_device_stats *mfhss_stats(struct net_device *dev);
// int dumHeader(struct sk_buff *pSkB, struct net_device *pDev, unsigned short type, 
// 	const void *pDAddr, const void *pSAddr, unsigned len);

//-------------------------------------------------------------------------------------------------
// Varibles
//-------------------------------------------------------------------------------------------------
struct net_device *mfhss_dev;
static void (*dummyNetdevInterrupt)(int, void *, struct pt_regs *);
static const struct net_device_ops mfhss_net_device_ops = {
 	.ndo_open            = mfhss_open,
 	.ndo_stop            = mfhss_close,
// 	.ndo_start_xmit      = dumTxPkt,
 	.ndo_do_ioctl        = mfhss_ioctl,
 	.ndo_set_config      = mfhss_config,
 	.ndo_get_stats       = mfhss_stats,
 	.ndo_change_mtu      = mfhss_change_mtu,
 	.ndo_tx_timeout      = mfhss_tx_timeout
};
// static const struct header_ops dummyHeaderOps = {
//     .create  = dumHeader,
//     .cache = NULL,
// };
static int timeout = DUMMY_NETDEV_TIMEOUT;
static unsigned long trans_start;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/*
 * Packet management functions
 */
static void pool_create(struct net_device *dev)
{
	int i;
	struct mfhss_priv_ *priv = netdev_priv(dev);
	struct mfhss_pkt_ *pkt;

	priv->pool = NULL;
	for (i = 0; i < POOL_SIZE; i++)
	{
		pkt = kmalloc(sizeof (struct mfhss_pkt_), GFP_KERNEL);
		if (pkt == NULL)
		{
			// PRINT_STATUS_MSG("cannot allocate packet #%i (%ld bytes)", -ENOMEM, i, sizeof (struct dumPacket_));
			return;
		}
		pkt->dev = dev;
		pkt->next = priv->pool;
		priv->pool = pkt;
	}
}

static void pool_destroy(struct net_device *dev) 
{
	struct mfhss_priv_ *priv = netdev_priv(dev);
	struct mfhss_pkt_ *pkt;

	while((pkt = priv->pool) != NULL) {
		priv->pool = pkt->next;
		kfree(pkt);
		// FIXME: in-flight packets (currently used)?
	}
}

static struct mfhss_pkt_ * pool_get(struct net_device *dev)
{
	unsigned long flags = 0;
	struct mfhss_priv_ *priv = netdev_priv(dev);
	struct mfhss_pkt_ *pkt;
	
	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->pool;
	if (pkt == NULL)
	{
		// PRINT_STATUS_MSG("pool is empty", -1);
		netif_stop_queue(dev);
	} else {
		priv->pool = pkt->next;
		pkt->next = NULL;
	}
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

static void pool_put(struct mfhss_pkt_ *pkt)
{
	unsigned long flags = 0;
	struct mfhss_priv_ *priv;
	
	if (pkt != NULL)
	{
		priv = netdev_priv(pkt->dev);
		spin_lock_irqsave(&priv->lock, flags);
		pkt->next = priv->pool;
		priv->pool = pkt;
		spin_unlock_irqrestore(&priv->lock, flags);
		if (netif_queue_stopped(pkt->dev) && pkt->next == NULL)
			netif_wake_queue(pkt->dev);
	}
}

static void enqueue_pkt(struct net_device *dev, struct mfhss_pkt_ *pkt)
{
	unsigned long flags = 0;
	struct mfhss_priv_ *priv = netdev_priv(dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;
	priv->rx_queue = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
}

static struct mfhss_pkt_ *dequeue_pkt(struct net_device *dev)
{
	unsigned long flags = 0;
	struct mfhss_pkt_ *pkt;
	struct mfhss_priv_ *priv = netdev_priv(dev);
	
	// spin_lock_irqsave(&pPriv->lock, flags);
	pkt = priv->rx_queue;
	if (pkt != NULL)
	{
		priv->rx_queue = pkt->next;
		pkt->next = NULL;
	}
	// spin_unlock_irqrestore(&pPriv->lock, flags);
	return pkt;
}

/*
 * Net device functionality support
 */
// static void rxIntEn(struct net_device *pDev, int bIsEnable)
// {
// 	struct dumPriv_ *pPriv = netdev_priv(pDev);
// 	pPriv->bIsRxIntEnabled = bIsEnable;
// }

// static void rxPkt(struct net_device *pDev, struct dumPacket_ *pPkt)
// {
// 	struct sk_buff *pSkB;
// 	struct dumPriv_ *pPriv = netdev_priv(pDev);

// 	// The packet has been retrived from transmission medium.
// 	// Build an skb around it, so upper layers can handle it
// 	pSkB = dev_alloc_skb(pPkt->datalen + 2);
// 	if (pSkB == NULL)
// 	{
// 		if (printk_ratelimit())
// 			PRINT_STATUS_MSG("cannot allocate socket buffer, packet dropped", -ENOMEM);
// 		pPriv->stats.rx_dropped++;
// 		return;			
// 	}
// 	skb_reserve(pSkB, 2); // align IP on 16-bit boundary
// 	memcpy(skb_put(pSkB, pPkt->datalen), pPkt->data, pPkt->datalen);

// 	// Write metadata, and then pass to the receive level
// 	pSkB->dev = pDev;
// 	pSkB->protocol = eth_type_trans(pSkB, pDev);
// 	pSkB->ip_summed = CHECKSUM_UNNECESSARY;
// 	pPriv->stats.rx_packets++;
// 	pPriv->stats.rx_bytes += pPkt->datalen;
// 	netif_rx(pSkB);
// }

// static void txPktByHW(char *pBuf, int len, struct net_device *pDev)
// {
// 	// This function deals with hw details. This interface loops back the packet to the other dummy interface (if any).
// 	// In other words, this function implements the dummy-device behaviour, while all other procedures are rather device-independent
// 	struct iphdr *pIP;
// 	struct net_device *pDest;
// 	struct dumPriv_ *pPriv;
// 	u32 *pSAddr, *pDAddr;
// 	struct dumPacket_ *pTxBuffer;
    
// 	// I am paranoid. Ain't I?
// 	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr))
// 	{
// 		PRINT_STATUS_MSG("packet too short (%i octets)", -1, len);
// 		return;
// 	}

	
// 	// Ethhdr is 14 bytes, but the kernel arranges for iphdr
// 	pIP = (struct iphdr *)(pBuf + sizeof(struct ethhdr));
// 	pSAddr = &pIP->saddr;
// 	pDAddr = &pIP->daddr;

// 	((u8 *)pSAddr)[2] ^= 1; // change the third octet (class C)
// 	((u8 *)pDAddr)[2] ^= 1;

// 	pIP->check = 0;         // and rebuild the checksum (ip needs it)
// 	pIP->check = ip_fast_csum((unsigned char *)pIP, pIP->ihl);

// 	if (pDev == dummyDevs[0])
// 		PDEBUG("%08x:%05i --> %08x:%05i\n",
// 				ntohl(pIP->saddr), ntohs(((struct tcphdr *)(pIP+1))->source),
// 				ntohl(pIP->daddr), ntohs(((struct tcphdr *)(pIP+1))->dest));
// 	else
// 		PDEBUG("%08x:%05i <-- %08x:%05i\n",
// 				ntohl(pIP->daddr), ntohs(((struct tcphdr *)(pIP+1))->dest),
// 				ntohl(pIP->saddr), ntohs(((struct tcphdr *)(pIP+1))->source));

	
// 	// Ok, now the packet is ready for transmission: first simulate a receive interrupt 
// 	// on the twin device, then a transmission-done on the transmitting device
// 	pDest = dummyDevs[pDev == dummyDevs[0] ? 1 : 0];
// 	pPriv = netdev_priv(pDest);
// 	pTxBuffer = getPkt(pDev);
// 	if (pTxBuffer != NULL)
// 	{
// 		int i;
		
// 		pTxBuffer->datalen = len;
// 	 	// fake transmit packet
// 		memcpy(pTxBuffer->data, pBuf, len);
// 		PDEBUG("hw tx packet by %s, len is %i", pDev->name, len);
		
// 	  	// fake receive packet
// 		enqueuePkt(pDest, pTxBuffer);	
// 		if (pPriv->bIsRxIntEnabled) 
// 		{
// 			pPriv->status |= DUMMY_NETDEV_RX_INTR;
// 		 	dummyNetdevInterrupt(0, pDest, NULL);
// 		}

// 		// terminate transmission
// 		pPriv = netdev_priv(pDev);
// 		pPriv->txPacketLen = len;
// 		pPriv->pTxPacketData = pBuf;
// 		pPriv->status |= DUMMY_NETDEV_TX_INTR;
// 		dummyNetdevInterrupt(0, pDev, NULL);
// 	}
// }


// static void regularIntHandler(int irq, void *pDevId, struct pt_regs *pRegs)
// {
// 	int statusWord;
// 	struct dumPriv_ *pPriv;
// 	struct dumPacket_ *pPkt = NULL;
	
// 	// As usual, check the "device" pointer to be sure it is really interrupting.
// 	// Then assign "struct device *dev"
// 	struct net_device *pDev = (struct net_device *)pDevId;
// 	// and check with hw if it's really ours 

// 	// paranoid
// 	if (pDev == NULL)
// 		return;

// 	// Lock the device
// 	pPriv = netdev_priv(pDev);
// 	spin_lock(&pPriv->lock);
// 	// retrieve statusword: real netdevices use I/O instructions
// 	statusWord = pPriv->status;
// 	// pPriv->status = 0;
// 	if (statusWord & DUMMY_NETDEV_RX_INTR) {
// 		PDEBUG("rx interrupt occur at %s", pDev->name);
// 		// send it to rxPkt for handling 
// 		pPkt = dequeuePkt(pDev);
// 		if (pPkt) {
// 			PDEBUG("received new packet at %s, len %i", pDev->name, pPkt->datalen);
// 			rxPkt(pDev, pPkt);
// 		}
// 	}
// 	if (statusWord & DUMMY_NETDEV_TX_INTR) {
// 		PDEBUG("tx interrupt occur at %s", pDev->name);
// 		// a transmission is over: free the skb
// 		pPriv->stats.tx_packets++;
// 		pPriv->stats.tx_bytes += pPriv->txPacketLen;
// 		dev_kfree_skb(pPriv->pSkB);
// 		pPriv->pSkB = NULL;
// 		statusWord &= ~DUMMY_NETDEV_TX_INTR;
// 		pPriv->status = statusWord;
// 	}

// 	// Unlock the device and we are done
// 	spin_unlock(&pPriv->lock);
// 	if (pPkt) 
// 		freePkt(pPkt); // Do this outside the lock!
// 	return;
// }

 
static void mfhss_setup(struct net_device *dev)
{
	// The init function (sometimes called probe).
	// It is invoked by register_netdev()
	struct mfhss_priv_ *priv = netdev_priv(dev);

	// Then, initialize the priv field. This encloses the statistics and a few private fields.

	// PDEBUG("try to setup device 0x%p, pPriv=0x%p", pDev, pPriv);
	
	memset(priv, 0, sizeof(struct mfhss_priv_));
	spin_lock_init(&priv->lock);
	priv->dev = dev;

#if 0
    // Make the usual checks: check_region(), probe irq, ...  -ENODEV
	// should be returned if no device found.  No resource should be
	// grabbed: this is done on open(). 
#endif 

	// Then, assign other fields in dev, using ether_setup() and some hand assignments
	ether_setup(dev);	// assign some of the fields
	dev->watchdog_timeo = timeout;
	
	// keep the default flags, just add NOARP
	dev->flags |= IFF_NOARP;
	dev->features |= NETIF_F_HW_CSUM;
	
	// dev->netdev_ops = &dummyNetdevOps;
	// dev->header_ops = &dummyHeaderOps;
	
	// rxIntEn(pDev, 1);	// enable receive interrupts
	pool_create(dev);
	// PRINT_STATUS(0);
}

static void mfhss_cleanup(void)
{
	// int i;
	// PRINT_STATUS(0);    
	// for (i = 0; i < 2;  i++) {
	// 	if (dummyDevs[i]) {
	// 		unregister_netdev(dummyDevs[i]);
	// 		destroyPool(dummyDevs[i]);
	// 		free_netdev(dummyDevs[i]);
	// 	}
	// }
}
	
/*
 * Net device operations
 */

static int mfhss_open(struct net_device *dev) 
{
	// request_region(), request_irq(), ...

	// Assign the hardware address
	memcpy(dev->dev_addr, "\0FHSS0", ETH_ALEN);
	PDEBUG("MAC address %02x:%02x:%02x:%02x:%02x:%02x assigned to %s", 
		dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5],
		dev->name);
	netif_start_queue(dev);
	// PRINT_STATUS(0);
	return 0;
}

static int mfhss_close(struct net_device *dev)
{
	// release ports, irq and such...

	netif_stop_queue(dev);
	return 0;	
}

static int mfhss_config(struct net_device *dev, struct ifmap *map)
{
	int err;

	// PDEBUG("configure the device %s", pDev->name);
	if (dev->flags & IFF_UP)
	{
		// can't act on a running interface
		PRINT_STATUS(err = -EBUSY);
		return err;
	} 

	if (map->base_addr != dev->base_addr) {
		// can't change I/O address
		PRINT_STATUS(err = -EOPNOTSUPP);
		return err;
	} 

	if (map->irq != dev->irq) {
		// Allow changing the IRQ
		dev->irq = map->irq;
	}

	// ignore other fields
	PRINT_STATUS(0);
	return 0;
}

// int dumTxPkt(struct sk_buff *pSkB, struct net_device *pDev)
// {
// 	int len;
// 	char *pData, shortpkt[ETH_ZLEN];
// 	struct dumPriv_ *pPriv = netdev_priv(pDev);
	
// 	pData = pSkB->data;
// 	len = pSkB->len;
// 	if (len < ETH_ZLEN) {
// 		memset(shortpkt, 0, ETH_ZLEN);
// 		memcpy(shortpkt, pSkB->data, pSkB->len);
// 		len = ETH_ZLEN;
// 		pData = shortpkt;
// 	}
// 	transStart = jiffies; 	// save the timestamp
// 	pPriv->pSkB = pSkB;				// Remember the skb, so we can free it at interrupt time

// 	// actual deliver of data is device-specific, and not shown here
// 	txPktByHW(pData, len, pDev);

// 	return NETDEV_TX_OK;
// }

static int mfhss_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
	// PRINT_STATUS_MSG("command 0x%04x not implemeted for %s", 0, cmd, pDev->name);
	return 0;
}

static int mfhss_change_mtu(struct net_device *dev, int new_mtu)
{
	// The "change_mtu" method is usually not needed.
	// If you need it, it must be like this.

	unsigned long flags = 0;
	int err = 0;
	struct mfhss_priv_ *priv = netdev_priv(dev);
	spinlock_t *plock = &priv->lock;
    
	// check ranges
	// PDEBUG("set new MTU=%i to %s", newMTU, pDev->name);
	if ((new_mtu < 68) || (new_mtu > 1500))
	{
		PRINT_STATUS_MSG("MTU is out if range (68, 1500): %i", (err=-EINVAL), new_mtu);
		return err;
	}

	// Do anything you need, and the accept the value
	spin_lock_irqsave(plock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(plock, flags);
	// PRINT_STATUS(0);
	return 0;
}

static void mfhss_tx_timeout (struct net_device *dev)
{
	struct mfhss_priv_ *priv = netdev_priv(dev);

	// PDEBUG("Transmit timeout at %ld, latency %ld\n", jiffies, jiffies - trans_start);
    
    // Simulate a transmission interrupt to get things moving
	// priv->status = DUMMY_NETDEV_TX_INTR;
	// dummyNetdevInterrupt(0, pDev, NULL);
	priv->stats->tx_errors++;
	netif_wake_queue(dev);
	return;
}

static struct net_device_stats *mfhss_stats(struct net_device *dev)
{
	// PDEBUG("getting device stats for %s", pDev->name);
	struct mfhss_priv_ *priv = netdev_priv(dev);
	return priv->stats;
}

/*
 * Header operations
 */
// int dumHeader(struct sk_buff *pSkB, struct net_device *pDev, unsigned short type, 
// 	const void *pDAddr, const void *pSAddr, unsigned len)
// {
// 	// PDEBUG("build ethernet header for %s", pDev->name);
// 	struct ethhdr *pEth = (struct ethhdr *)skb_push(pSkB, ETH_HLEN);

// 	pEth->h_proto = htons(type);
// 	memcpy(pEth->h_source, pSAddr ? pSAddr : pDev->dev_addr, pDev->addr_len);
// 	memcpy(pEth->h_dest, pDAddr ? pDAddr : pDev->dev_addr, pDev->addr_len);
// 	pEth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
// 	return (pDev->hard_header_len);
// }

/*
 * Entry/exit point functions
 */
static int mfhss_init(void)
{
	int err = 0, i;

	// Define interrupt
	// dummyNetdevInterrupt = regularIntHandler;

	// Allocate devices
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0))
	mfhss_dev = alloc_netdev(sizeof (struct mfhss_priv_), "dm%d", mfhss_setup);
#else
	mfhss_dev = alloc_netdev(sizeof (struct mfhss_priv_), "dm%d", NET_NAME_UNKNOWN, mfhss_setup);
#endif
	if (mfhss_dev == NULL)
	{
		err = -ENOMEM;
		// PRINT_STATUS_MSG("cannot allocate struct dummyNet_priv_ (err=%d)", err, err);
		// dumCleanup();
		return err;
	} else {
		// PDEBUG("mfhss_dev allocated at 0x%p (%ld bytes)", i, mfhss_dev, sizeof *mfhss_dev);
	}

	// Register devices
	if ((err = register_netdev(mfhss_dev)) != 0)
	{
		// PRINT_STATUS_MSG("cannot register net device (err=%d)", err, err);
		// dumCleanup();
		return err;
	} else {
		PDEBUG("mfhss_dev successfully registered!");
	}

	//PRINT_STATUS(err);
	return 0;
}

static void mfhss_exit(void)
{	
	// dumCleanup();
	// PRINT_STATUS(0);
}

module_init(mfhss_init);
module_exit(mfhss_cleanup);