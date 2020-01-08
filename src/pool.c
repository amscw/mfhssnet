#include "mfhssnet.h"
#include "pool.h"

//-------------------------------------------------------------------------------------------------
// Varibles
//-------------------------------------------------------------------------------------------------
static LIST_HEAD(pool_list);
static spinlock_t lock;
static size_t count;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
/*
 * Packet management functions
 */
int pool_create(struct net_device *dev, size_t maxlen)
{
	int i, err = 0;
	struct mfhss_pkt_ *pkt;

	INIT_LIST_HEAD(&pool_list);
	for (i = 0; i < POOL_SIZE; i++)
	{
		pkt = kmalloc(sizeof (struct mfhss_pkt_), GFP_KERNEL);
		if (pkt == NULL)
		{
			err = -ENOMEM;
			break;
		} else {
			pkt->data = kmalloc(maxlen, GFP_KERNEL);
			if (pkt->data == NULL)
			{
				err = -ENOMEM;
				kfree(pkt);
				break;
			}
		}
		pkt->dev = dev;
		pkt->datalen = 0;
		list_add(&pkt->list, &pool_list); // now pool_list.next == &pkt->list
		// PDEBUG("&pkt->list=%p, pool_list.next=%p, pool_list.prev=%p\n", &pkt->list, pool_list.next, pool_list.prev);
	}
	count = i;
	spin_lock_init(&lock);
	PDEBUG("created %li packets in pool", count);
	return err;
}

void pool_destroy(void) 
{
	struct mfhss_pkt_ *pkt;
	struct list_head *p;
	int i = 0;

	list_for_each(p, &pool_list)
	{
		pkt = list_entry(p, struct mfhss_pkt_, list);
		if (pkt->data != NULL)
		{
			kfree(pkt->data);
			pkt->data = NULL;
		}
		kfree(pkt);
		i++;
	}
	count -= i;
	PDEBUG("destroyed %i packets in pool\n", i);
	// FIXME: in-flight packets (currently used)?
}

struct mfhss_pkt_ *pool_get(void)
{
	unsigned long flags = 0;
	struct mfhss_pkt_ *pkt;
	
	if (pool_list.next == &pool_list)
	{
		PDEBUG("pool is empty!\n");
		pkt = NULL;
	} else {
		spin_lock_irqsave(&lock, flags);
		// PDEBUG("next=%p, prev=%p, list=%p", pool_list.next, pool_list.prev, &pool_list);
		pkt = list_entry(pool_list.next, struct mfhss_pkt_, list);
		list_del(&pkt->list); 
		count--;
		spin_unlock_irqrestore(&lock, flags);
	}
	return pkt;
}

size_t pool_put(struct mfhss_pkt_ *pkt)
{
	unsigned long flags = 0;
	
	if (pkt != NULL)
	{
		memset(pkt->data, 0, pkt->datalen);
		pkt->dev = NULL;
		pkt->datalen = 0;
		spin_lock_irqsave(&lock, flags);
		list_add(&pkt->list, &pool_list);
		count++;
		spin_unlock_irqrestore(&lock, flags);
	}
	return count;
}

// void enqueue_pkt(struct net_device *dev, struct mfhss_pkt_ *pkt)
// {
// 	unsigned long flags = 0;
// 	struct mfhss_priv_ *priv = netdev_priv(dev);

// 	spin_lock_irqsave(&priv->lock, flags);
// 	pkt->next = priv->rx_queue;
// 	priv->rx_queue = pkt;
// 	spin_unlock_irqrestore(&priv->lock, flags);
// }

// struct mfhss_pkt_ *dequeue_pkt(struct net_device *dev)
// {
// 	unsigned long flags = 0;
// 	struct mfhss_pkt_ *pkt;
// 	struct mfhss_priv_ *priv = netdev_priv(dev);
	
// 	// spin_lock_irqsave(&pPriv->lock, flags);
// 	pkt = priv->rx_queue;
// 	if (pkt != NULL)
// 	{
// 		priv->rx_queue = pkt->next;
// 		pkt->next = NULL;
// 	}
// 	// spin_unlock_irqrestore(&pPriv->lock, flags);
// 	return pkt;
// }
