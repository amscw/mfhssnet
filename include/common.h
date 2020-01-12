#ifndef _COMMON_H
#define _COMMON_H

#include <linux/kernel.h> 		// printk(), abs()
#include <linux/slab.h>			// kmalloc()
#include <linux/errno.h>		// error codes
#include <linux/types.h>		// size_t
#include <linux/spinlock.h>		// spinlock_t
#include <linux/version.h>
#include <linux/list.h>
#include <linux/ioport.h>		// FIXME: exclude it from here, need for struct resource

//-------------------------------------------------------------------------------------------------
// MACRO
//-------------------------------------------------------------------------------------------------
#define DRIVER_NAME "mfhssnet"
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s(%s):"fmt, DRIVER_NAME, __FUNCTION__, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s(%s):"fmt, DRIVER_NAME, __FUNCTION__, ##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)

#define MAX_ERR	35
#define PRINT_ERR(err) {\
	int tmp = (err);\
	int index = abs(tmp);\
	if (index > MAX_ERR) index = MAX_ERR;\
	printk(KERN_ERR"%s(%s)-[%s/%02i]\n", DRIVER_NAME, __FUNCTION__,\
		err_strings[index], tmp);\
}
#define PRINT_ERR_MSG(err) {\
	int tmp = (err);\
	int index = abs(tmp);\
	if (index > MAX_ERR) index = MAX_ERR;\
	printk(KERN_ERR"%s(%s)-[%s/%02i] %s\n", DRIVER_NAME, __FUNCTION__,\
		err_strings[index], tmp, err_messages[index]);\
}

//-------------------------------------------------------------------------------------------------
// Varibles
//-------------------------------------------------------------------------------------------------
extern const char* const err_strings[];
extern const char* const err_messages[];

#endif // _COMMON_H
