#ifndef _MFHSSIOCTL_H
#define _MFHSSIOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define REG_NAME_SIZE	32

typedef struct
{
	/* const */ char regName[REG_NAME_SIZE];
	/* const */ char targetNode[REG_NAME_SIZE];
	unsigned int address;
} __attribute__((__packed__)) MFHSS_REG_TypeDef;

typedef struct
{
	/* const */ char nodeName[REG_NAME_SIZE];
} MFHSS_DIR_TypeDef;

/* Use 'm' as mfhssdrv magic number */
#define MFHSSNET_IOC_MAGIC 'm'

/* SPIDRV commands */
#define MFHSSNET_IORESET 		_IO(MFHSSNET_IOC_MAGIC, 0)
#define MFHSSNET_IOMAKEREG 		_IOW(MFHSSNET_IOC_MAGIC, 1, MFHSS_REG_TypeDef)
#define MFHSSNET_IOMAKEGROUP	_IOW(MFHSSNET_IOC_MAGIC, 2, MFHSS_DIR_TypeDef)

#define MFHSSNET_IOC_MAXNR 3

#endif // _MFHSSIOCTL_H