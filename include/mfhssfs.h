#ifndef _MFHSSFS_H
#define _MFHSSFS_H

#include "common.h"
#include <linux/sysfs.h>
#include <linux/kobject.h>

//-------------------------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------------------------
extern struct attribute *DMA_attributes[];
extern struct attribute *MLIP_attributes[];

#endif // _MFHSSFS_H