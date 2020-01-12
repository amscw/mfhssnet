#ifndef _MFHSSIOCTL_H
#define _MFHSSIOCTL_H

#include <linux/types.h>

// select type of driver
#define DRV_TYPE_NET
// #define DRV_TYPE_CHAR

#if defined(DRV_TYPE_CHAR)
#include <linux/ioctl.h>
#elif defined(DRV_TYPE_NET)
#include <linux/sockios.h>
#endif

#define REG_NAME_SIZE	32

typedef struct
{
	/* const */ char regName[REG_NAME_SIZE];
	/* const */ char targetNode[REG_NAME_SIZE];
	unsigned int address;
} __attribute__((__packed__)) MFHSS_FILE_TypeDef;

typedef struct
{
	/* const */ char nodeName[REG_NAME_SIZE];
} MFHSS_DIR_TypeDef;

#if defined(DRV_TYPE_CHAR)
/* Use 'm' as mfhssdrv magic number */
#define MFHSS_IOC_MAGIC 'm'

/* modem-fhss commands */
#define MFHSS_IORESET		_IO(MFHSSNET_IOC_MAGIC, 0)
#define MFHSS_IOMAKEFILE 	_IOW(MFHSSNET_IOC_MAGIC, 1, MFHSS_REG_TypeDef)
#define MFHSS_IOMAKEDIR		_IOW(MFHSSNET_IOC_MAGIC, 2, MFHSS_DIR_TypeDef)

#define MFHSS_IOC_MAXNR 3

#elif defined(DRV_TYPE_NET)
#define MFHSS_IORESET			(SIOCDEVPRIVATE)
#define MFHSS_IOMAKEFILE		(SIOCDEVPRIVATE + 1)
#define MFHSS_IOMAKEDIR			(SIOCDEVPRIVATE + 2)
#endif

#endif // _MFHSSIOCTL_H
