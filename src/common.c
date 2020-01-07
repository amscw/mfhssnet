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
};

const char * const err_messages[] = {
	"No error",
	"Operation not permitted",
	"No such file or directory",
	"No such process",
	"Interrupted system call",
	"I/O error",
	"No such device or address",
	"Argument list too long",
	"Exec format error",
	"Bad file number",
	"No child processes",
	"Try again",
	"Out of memory",
	"Permission denied",
	"Bad address",
	"Block device required",
	"Device or resource busy",
	"File exists",
	"Cross-device link",
	"No such device",
	"Not a directory",
	"Is a directory",
	"Invalid argument",
	"File table overflow",
	"Too many open files",
	"Not a typewriter",
	"Text file busy",
	"File too large",
	"No space left on device",
	"Illegal seek",
	"Read-only file system",
	"Too many links",
	"Broken pipe",
	"Math argument out of domain of func",
	"Math result not representable"
};
