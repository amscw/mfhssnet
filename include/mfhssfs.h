#ifndef _MFHSSFS_H
#define _MFHSSFS_H

#include "common.h"
#include <linux/sysfs.h>
#include <linux/kobject.h>

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Prototypes
//-------------------------------------------------------------------------------------------------
int create_dma_subdir(struct kset* dir, struct mfhss_priv_ *priv);
int create_mlip_subdir(struct kset* dir, struct mfhss_priv_ *priv);
inline void clean_dir(struct kset *dir);
int create_file(struct kobject *dir, const char *name, unsigned address);
int create_dir(struct kset *dir, struct mfhss_priv_ *priv, const char *name);

#endif // _MFHSSFS_H