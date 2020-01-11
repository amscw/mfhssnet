const char * const err_strings[] = {
	"OK",
	"EPERM",		/* 1  */
	"ENOENT",		/* 2  */
	"ESRCH",		/* 3  */
	"EINTR",		/* 4  */
	"EIO",		 	/* 5  */
	"ENXIO",		/* 6  */
	"E2BIG",		/* 7  */
	"ENOEXEC",		/* 8  */
	"EBADF",		/* 9  */
	"ECHILD",		/* 10 */
	"EAGAIN",		/* 11 */
	"ENOMEM",		/* 12 */
	"EACCES",		/* 13 */
	"EFAULT",		/* 14 */
	"ENOTBLK",		/* 15 */
	"EBUSY",		/* 16 */
	"EEXIST",		/* 17 */
	"EXDEV",		/* 18 */
	"ENODEV",		/* 19 */
	"ENOTDIR",		/* 20 */
	"EISDIR",		/* 21 */
	"INVAL",		/* 22 */
	"ENFILE",		/* 23 */
	"EMFILE",		/* 24 */
	"ENOTTY",		/* 25 */
	"ETXTBSY",		/* 26 */
	"EFBIG",		/* 27 */
	"ENOSPC",		/* 28 */
	"ESPIPE",		/* 29 */
	"EROFS",		/* 30 */
	"EMLINK",		/* 31 */
	"EPIPE",		/* 32 */
	"EDOM",			/* 33 */
	"ERANGE",		/* 34 */
	"unknown",
};

const char * const err_messages[] = {
	"No error",
	"Operation not permitted", 		/* 1  */
	"No such file or directory",	/* 2  */
	"No such process",				/* 3  */
	"Interrupted system call",		/* 4  */
	"I/O error",					/* 5  */
	"No such device or address",	/* 6  */
	"Argument list too long",		/* 7  */
	"Exec format error",			/* 8  */
	"Bad file number",				/* 9  */
	"No child processes",			/* 10 */
	"Try again",					/* 11 */
	"Out of memory",				/* 12 */
	"Permission denied",			/* 13 */
	"Bad address",					/* 14 */
	"Block device required",		/* 15 */
	"Device or resource busy",		/* 16 */
	"File exists",					/* 17 */
	"Cross-device link",			/* 18 */
	"No such device",				/* 19 */
	"Not a directory",				/* 20 */
	"Is a directory",				/* 21 */
	"Invalid argument",				/* 22 */
	"File table overflow",			/* 23 */
	"Too many open files",			/* 24 */
	"Not a typewriter",				/* 25 */
	"Text file busy",				/* 26 */
	"File too large",				/* 27 */
	"No space left on device",		/* 28 */
	"Illegal seek",					/* 29 */
	"Read-only file system",		/* 30 */
	"Too many links",				/* 31 */
	"Broken pipe",					/* 32 */
	"Math argument out of domain of func",	/* 33 */
	"Math result not representable",	/* 34 */
	"description unavailable, see uapi/asm-generic/errno.h",	/* 35 */
};
