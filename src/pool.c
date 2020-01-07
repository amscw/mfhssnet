#include "mfhssnet.h"
#include "pool.h"
#include <linux/netdevice.h> // выкинуть нахер отсюда, ввести дескриптор пула 

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/*
 * Packet management functions
 */
int pool_create(struct net_device *dev, size_t datalen)
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
			pool_destroy(dev);
			return -ENOMEM;
		} else {
			pkt->data = kmalloc(datalen, GFP_KERNEL);
			if (pkt->data == NULL)
			{
				pool_destroy(dev);
				return -ENOMEM;
			}
		}
		pkt->dev = dev;
		pkt->next = priv->pool;
		priv->pool = pkt;
	}
	return 0;
}

void pool_destroy(struct net_device *dev) 
{
	struct mfhss_priv_ *priv = netdev_priv(dev);
	struct mfhss_pkt_ *pkt;

	while((pkt = priv->pool) != NULL) 
	{
		priv->pool = pkt->next;
		if (pkt->data != NULL)
			kfree(pkt->data);
		kfree(pkt);
		// FIXME: in-flight packets (currently used)?
	}
}

struct mfhss_pkt_ * pool_get(struct net_device *dev)
{
	unsigned long flags = 0;
	struct mfhss_priv_ *priv = netdev_priv(dev);
	struct mfhss_pkt_ *pkt;
	
	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->pool;
	if (pkt == NULL)
	{
		// no packets available
		netif_stop_queue(dev);
	} else {
		priv->pool = pkt->next;
		pkt->next = NULL;
	}
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

void pool_put(struct mfhss_pkt_ *pkt)
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

void enqueue_pkt(struct net_device *dev, struct mfhss_pkt_ *pkt)
{
	unsigned long flags = 0;
	struct mfhss_priv_ *priv = netdev_priv(dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;
	priv->rx_queue = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
}

struct mfhss_pkt_ *dequeue_pkt(struct net_device *dev)
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
