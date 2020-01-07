#ifndef _COMMON_H
#define _COMMON_H

#include <linux/kernel.h> 		// printk()
#include <linux/slab.h>			// kmalloc()
#include <linux/errno.h>		// error codes
#include <linux/types.h>		// size_t
#include <linux/spinlock.h>		// spinlock_t
#include <linux/version.h>

//-------------------------------------------------------------------------------------------------
// MACRO
//-------------------------------------------------------------------------------------------------
#define DRIVER_NAME "dummy-net"
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s(%s):"fmt, DRIVER_NAME, __FUNCTION__, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s(%s):"fmt, DRIVER_NAME, __FUNCTION__, ##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)

#define MAX_ERR	35
#define PRINT_ERR(err) printk(KERN_ERR"%s(%s)-[%02d(%s)]\n", DRIVER_NAME, __FUNCTION__, err,\
	(err < MAX_ERR) ? err_strings[err] : "unknown")
#define PRINT_ERR_MSG(err) printk(KERN_ERR"%s(%s)-[%02d(%s)] %s\n", DRIVER_NAME, __FUNCTION__, err,\
	(err < MAX_ERR) ? err_strings[err] : "unknown", (err < MAX_ERR) ? err_messages[err] : "description unavailable, see uapi/asm-generic/errno.h")

//-------------------------------------------------------------------------------------------------
// Varibles
//-------------------------------------------------------------------------------------------------
extern const char* const err_strings[];
extern const char* const err_messages[];

#endif // _COMMON_H
