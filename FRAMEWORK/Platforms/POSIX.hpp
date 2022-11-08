
#pragma once

#define PXCALL __cdecl
//============================================================================================================
// https://docs.oracle.com/cd/E19048-01/chorus5/806-6897/auto1/index.html
// https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
// https://man7.org/linux/man-pages/man2/syscalls.2.html
// https://filippo.io/linux-syscall-table/
// https://marcin.juszkiewicz.com.pl/download/tables/syscalls.html

// Only most useful POSIX functions will go here for now
// NOTE: We should be able to use these definitions to call X64 functions from X32 code if necessary ???
template<typename PHT> struct NPOSIX  // For members: alignas(sizeof(PHT))
{
 using PVOID    = SPTR<void,  PHT>;
 using PCHAR    = SPTR<achar, PHT>;
//using HANDLE   = PVOID;
 using SIZE_T   = SPTR<uint,  PHT>;
 using SSIZE_T  = SPTR<sint,  PHT>;

//using LONG     = int32;
//using ULONG    = uint32;
//using PSIZE_T  = SIZE_T*;
//using PULONG   = ULONG*;
//using NTSTATUS = LONG;
 using  mode_t  = uint32;

enum EDFD  // These are just for convenience. These descriptors don`t have to be open on every system (Android?)
{
 STDIN,
 STDOUT,
 STDERR
};

enum EErrs   // Linux
{
	EPERM		 = 1  , // Operation not permitted
	ENOENT		 = 2	, // No such file or directory
	ESRCH		 = 3	, // No such process
	EINTR		 = 4	, // Interrupted system call
	EIO		 = 5	, // I/O error
	ENXIO		 = 6	, // No such device or address
	E2BIG		 = 7	, // Arg list too long
	ENOEXEC		 = 8	, // Exec format error
	EBADF		 = 9	, // Bad file number
	ECHILD		= 10	, // No child processes
	EAGAIN		= 11	, // Try again
	ENOMEM		= 12	, // Out of memory
	EACCES		= 13	, // Permission denied
	EFAULT		= 14	, // Bad address
	ENOTBLK		= 15	, // Block device required
	EBUSY		= 16	, // Device or resource busy
	EEXIST		= 17	, // File exists
	EXDEV		= 18	, // Cross-device link
	ENODEV		= 19	, // No such device
	ENOTDIR		= 20	, // Not a directory
	EISDIR		= 21	, // Is a directory
	EINVAL		= 22	, // Invalid argument
	ENFILE		= 23	, // File table overflow
	EMFILE		= 24	, // Too many open files
	ENOTTY		= 25	, // Not a typewriter
	ETXTBSY		= 26	, // Text file busy
	EFBIG		= 27	, // File too large
	ENOSPC		= 28	, // No space left on device
	ESPIPE		= 29	, // Illegal seek
	EROFS		= 30	, // Read-only file system
	EMLINK		= 31	, // Too many links
	EPIPE		= 32	, // Broken pipe
	EDOM		= 33	, // Math argument out of domain of func
	ERANGE		= 34	, // Math result not representable
	EDEADLK		= 35	, // Resource deadlock would occur
	ENAMETOOLONG = 36	, // File name too long
	ENOLCK		= 37	, // No record locks available
	ENOSYS		= 38	, // Function not implemented
	ENOTEMPTY	= 39	, // Directory not empty
	ELOOP		= 40	, // Too many symbolic links encountered
	EWOULDBLOCK	= 41	, // Operation would block
	ENOMSG		= 42	, // No message of desired type
	EIDRM		= 43	, // Identifier removed
	ECHRNG		= 44	, // Channel number out of range
	EL2NSYNC	= 45	, // Level 2 not synchronized
	EL3HLT		= 46	, // Level 3 halted
	EL3RST		= 47	, // Level 3 reset
	ELNRNG		= 48	, // Link number out of range
	EUNATCH		= 49	, // Protocol driver not attached
	ENOCSI		= 50	, // No CSI structure available
	EL2HLT		= 51	, // Level 2 halted
	EBADE		= 52	, // Invalid exchange
	EBADR		= 53	, // Invalid request descriptor
	EXFULL		= 54	, // Exchange full
	ENOANO		= 55	, // No anode
	EBADRQC		= 56	, // Invalid request code
	EBADSLT		= 57	, // Invalid slot
	EDEADLOCK	= 58	, // File locking deadlock error
	EBFONT		= 59	, // Bad font file format
	ENOSTR		= 60	, // Device not a stream
	ENODATA		= 61	, // No data available
	ETIME		= 62	, // Timer expired
	ENOSR		= 63	, // Out of streams resources
	ENONET		= 64	, // Machine is not on the network
	ENOPKG		= 65	, // Package not installed
	EREMOTE		= 66	, // Object is remote
	ENOLINK		= 67	, // Link has been severed
	EADV		= 68	, // Advertise error
	ESRMNT		= 69	, // Srmount error
	ECOMM		= 70	, // Communication error on send
	EPROTO		= 71	, // Protocol error
	EMULTIHOP	= 72	, // Multihop attempted
	EDOTDOT		= 73	, // RFS specific error
	EBADMSG		= 74	, // Not a data message
	EOVERFLOW	= 75	, // Value too large for defined data type
	ENOTUNIQ	= 76	, // Name not unique on network
	EBADFD		= 77	, // File descriptor in bad state
	EREMCHG		= 78	, // Remote address changed
	ELIBACC		= 79	, // Can not access a needed shared library
	ELIBBAD		= 80	, // Accessing a corrupted shared library
	ELIBSCN		= 81	, // .lib section in a.out corrupted
	ELIBMAX		= 82	, // Attempting to link in too many shared libraries
	ELIBEXEC	= 83	, // Cannot exec a shared library directly
	EILSEQ		= 84	, // Illegal byte sequence
	ERESTART	= 85	, // Interrupted system call should be restarted
	ESTRPIPE	= 86	, // Streams pipe error
	EUSERS		= 87	, // Too many users
	ENOTSOCK	= 88	, // Socket operation on non-socket
	EDESTADDRREQ	= 89	, // Destination address required
	EMSGSIZE	= 90	, // Message too long
	EPROTOTYPE	= 91	, // Protocol wrong type for socket
	ENOPROTOOPT	= 92	, // Protocol not available
	EPROTONOSUPPORT	= 93	, // Protocol not supported
	ESOCKTNOSUPPORT	= 94	, // Socket type not supported
	EOPNOTSUPP	= 95	, // Operation not supported on transport endpoint
	EPFNOSUPPORT	= 96	, // Protocol family not supported
	EAFNOSUPPORT	= 97	, // Address family not supported by protocol
	EADDRINUSE	= 98	, // Address already in use
	EADDRNOTAVAIL	= 99	, // Cannot assign requested address
	ENETDOWN	= 100	, // Network is down
	ENETUNREACH	= 101	, // Network is unreachable
	ENETRESET	= 102	, // Network dropped connection because of reset
	ECONNABORTED	= 103	, // Software caused connection abort
	ECONNRESET	= 104	, // Connection reset by peer
	ENOBUFS		= 105	, // No buffer space available
	EISCONN		= 106	, // Transport endpoint is already connected
	ENOTCONN	= 107	, // Transport endpoint is not connected
	ESHUTDOWN	= 108	, // Cannot send after transport endpoint shutdown
	ETOOMANYREFS	= 109	, // Too many references: cannot splice
	ETIMEDOUT	= 110	, // Connection timed out
	ECONNREFUSED	= 111	, // Connection refused
	EHOSTDOWN	= 112	, // Host is down
	EHOSTUNREACH	= 113	, // No route to host
	EALREADY	= 114	, // Operation already in progress
	EINPROGRESS	= 115	, // Operation now in progress
	ESTALE		= 116	, // Stale NFS file handle
	EUCLEAN		= 117	, // Structure needs cleaning
	ENOTNAM		= 118	, // Not a XENIX named type file
	ENAVAIL		= 119	, // No XENIX semaphores available
	EISNAM		= 120	, // Is a named type file
	EREMOTEIO	= 121	, // Remote I/O error
};


// http://cqctworld.org/src/l1/lib/linux-x86-enum.names
// https://man7.org/linux/man-pages/man2/open.2.html
// https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2uapi_2asm-generic_2fcntl_8h.html
// https://opensource.apple.com/source/xnu/xnu-344/bsd/sys/fcntl.h
// https://github.com/dlang/druntime/blob/master/src/core/sys/posix/fcntl.d

enum EOpnFlg    // For 'flags' field    // Platform/Arch dependant?
{
 O_RDONLY    = 0x00000000,    // Open for reading only.
 O_WRONLY    = 0x00000001,    // Open for writing only.
 O_RDWR      = 0x00000002,    // Open for reading and writing.

 O_CREAT     = 0x00000040,    // If the file exists, this flag has no effect. Otherwise, the owner ID of the file is set to the user ID of the c_actor, the group ID of the file is set to the group ID of the c_actor, and the low-order 12 bits of the file mode are set to the value of mode.
 O_EXCL      = 0x00000080,    // If O_EXCL and O_CREAT are set, open will fail if the file exists.

 O_NOCTTY    = 0x00000100,
 O_TRUNC     = 0x00000200,    // If the file exists, its length is truncated to 0 and the mode and owner are unchanged.
 O_APPEND    = 0x00000400,    // If set, the file pointer will be set to the end of the file prior to each write.
 O_NONBLOCK  = 0x00000800,    // If O_NONBLOCK is set, the open will return without waiting for the device to be ready or available. Subsequent behavior of the device is device-specific.
// Additional:
 O_DSYNC     = 0x00001000,
 O_DIRECT    = 0x00004000,    // direct disk access hint
 O_LARGEFILE = 0x00008000,
 O_DIRECTORY = 0x00010000,    // must be a directory
 O_NOFOLLOW  = 0x00020000,    // don't follow links
 O_NOATIME   = 0x00040000,
 O_CLOEXEC   = 0x00080000,    // set close_on_exec
 O_PATH      = 0x00200000,
 O_TMPFILE   = 0x00410000,
 O_SYNC      = 0x00101000,
};

enum EMode
{
 S_IXOTH = 0x00000001, // 00001 // others have execute permission
 S_IWOTH = 0x00000002, // 00002 // others have write permission
 S_IROTH = 0x00000004, // 00004 // others have read permission
 S_IRWXO = 0x00000007, // 00007 // others have read, write, and execute permission

 S_IXGRP = 0x00000008, // 00010 // group has execute permission
 S_IWGRP = 0x00000010, // 00020 // group has write permission
 S_IRGRP = 0x00000020, // 00040 // group has read permission
 S_IRWXG = 0x00000038, // 00070 // group has read, write, and execute permission
 S_IXUSR = 0x00000040, // 00100 // user has execute permission
 S_IWUSR = 0x00000080, // 00200 // user has write permission
 S_IRUSR = 0x00000100, // 00400 // user has read permission
 S_IRWXU = 0x000001C0, // 00700 // user (file owner) has read, write, and execute permission

// According to POSIX, the effect when other bits are set in mode is unspecified.  On Linux, the following bits are also honored in mode:
 S_ISVTX = 0x00000200, // 01000 // sticky bit (see inode(7)).
 S_ISGID = 0x00000400, // 02000 // set-group-ID bit (see inode(7)).
 S_ISUID = 0x00000800, // 04000 // set-user-ID bit
};

// Terminates the calling process "immediately".  Any open file descriptors belonging to the process are closed.  Any children of the process are inherited by init(1) (or by the nearest "subreaper" process as defined through the use of the prctl(2) PR_SET_CHILD_SUBREAPER operation).  The process's parent is sent a SIGCHLD signal.
// The value status & 0xFF is returned to the parent process as the process's exit status, and can be collected by the parent using one of the wait(2) family of calls.
// The raw _exit() system call terminates only the calling thread, and actions such as reparenting child processes or sending SIGCHLD to the parent process are performed only if this is the last thread in the thread group.
static void PXCALL exit(int status);

// This system call is equivalent to _exit(2) except that it terminates not only the calling thread, but all threads in the calling process's thread group.
static void PXCALL exit_group(int status);

// The path parameter points to a path name naming a file. The open function opens a file descriptor for the named file and sets the file status flags according to the value of oflag.
static int PXCALL open(PCHAR pathname, int flags, mode_t mode);  // 'open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode)' is same as call to 'creat'

// Equivalent to: open(path, O_WRONLY|O_CREAT|O_TRUNC, mode)
static int PXCALL creat(const char *path, mode_t mode);    // Use it as fallback if a correct O_CREAT cannot be found

// The fildes field contains a file descriptor obtained from an open(2POSIX), dup(2POSIX), accept(2POSIX), socket(2POSIX), or shm_open(2POSIX) system call. The close function closes the file descriptor indicated by fildes.
static int PXCALL close(int fildes);

// Creates a filesystem node (file, device special file, or named pipe) named pathname, with attributes specified by mode and dev.
// static int PXCALL mknod(const char *pathname, mode_t mode, dev_t dev);

struct iovec
{
 PVOID  iov_base;   // base address
 SIZE_T iov_len;    // length
};

// Attempts to read nbytes of data from the object referenced by the descriptor d into the buffer pointed to by buf . readv() performs the same action, but scatters the input data into the iovcnt buffers specified by the members of the iov array: iov[0] , iov[1] , ..., iov[iovcnt-1] .
// Upon successful completion, read() and readv() return the number of bytes actually read and placed in the buffer. The system guarantees to read the number of bytes requested if the descriptor references a normal file that contains that many bytes before the end-of-file, but in no other case. Upon end-of-file, 0 is returned. read() and readv() also return 0 if a non-blocking read is attempted on a socket that is no longer open. Otherwise, -1 is returned, and the global variable errno is set to indicate the error.
static SSIZE_T PXCALL read(int fd, PVOID buf, SIZE_T nbytes);
static SSIZE_T PXCALL readv(int fd, iovec* iov, int iovcnt);

// Attempts to write nbytes of data to the object referenced by the descriptor d from the buffer pointed to by buf . writev() performs the same action, but gathers the output data from the iovcnt buffers specified by the members of the iov array: iov[0] , iov[1] , ..., iov[iovcnt-1] .
// Upon successful completion, write() and writev() return the number of bytes actually written. Otherwise, they return -1 and set errno to indicate the error.
static SSIZE_T PXCALL write(int fd, PVOID buf, SIZE_T nbytes);
static SSIZE_T PXCALL writev(int fd, iovec* iov, int iovcnt);     // Windows: WriteFileGather

enum ESeek
{
 SEEK_SET  = 0,       // Seek relative to begining of file
 SEEK_CUR  = 1,       // Seek relative to current file position
 SEEK_END  = 2,       // Seek relative to end of file
};

// Repositions the file offset of the open file description associated with the file descriptor fd to the argument offset according to the directive whence
static SSIZE_T PXCALL lseek(int fd, SSIZE_T offset, int whence);

// x32 only(Not present on x64)!
static int PXCALL llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low, uint64* result, unsigned int whence);

// Attempts to create a directory named pathname.
static int PXCALL mkdir(PCHAR pathname, mode_t mode);

// Deletes a name from the filesystem.  If that name was the last link to a file and no processes have the file open, the file is deleted and the space it was using is made available for reuse.
// If the name was the last link to a file but any processes still have the file open, the file will remain in existence until the last file descriptor referring to it is closed.
// If the name referred to a symbolic link, the link is removed.
// If the name referred to a socket, FIFO, or device, the name for it is removed but processes which have the object open may continue to use it.
static int PXCALL unlink(PCHAR pathname);

// Deletes a directory, which must be empty
static int PXCALL rmdir(PCHAR pathname);

// Renames a file, moving it between directories if required.  Any other hard links to the file (as created using link(2)) are unaffected.  Open file descriptors for oldpath are also unaffected.
static int PXCALL rename(PCHAR oldpath, PCHAR newpath);

// Places the contents of the symbolic link pathname in the buffer buf, which has size bufsiz.  readlink() does not append a terminating null byte to buf.  It will (silently) truncate the contents (to a length of bufsiz characters), in case the buffer is too small to hold all of the contents.
// On success return the number of bytes placed in buf. (If the returned value equals bufsiz, then truncation may have occurred.)
static ssize_t PXCALL readlink(PCHAR pathname, PCHAR buf, SIZE_T bufsiz);

//TODO: 'int fcntl(int fd, int cmd, void* arg)' for a directory change notification

enum EAcss
{
 F_OK	=	0,   	// test for existence of file
 X_OK	=	0x01,	// test for execute or search permission
 W_OK	=	0x02,	// test for write permission
 R_OK	=	0x04,	// test for read permission
};

// Checks whether the calling process can access the file pathname.  If pathname is a symbolic link, it is dereferenced.
static int PXCALL access(PCHAR pathname, int mode);


// /arch/{ARCH}/include/uapi/asm/stat.h
// https://stackoverflow.com/questions/29249736/what-is-the-precise-definition-of-the-structure-passed-to-the-stat-system-call
//
struct SFStat    // Too volatile to use crossplatform?
{
 SIZE_T    st_dev;        // inode's device
 SIZE_T    st_ino;        // inode's number
 SIZE_T    st_nlink;      // number of hard links

 unsigned int  de;        // inode protection mode
 unsigned int  st_uid;    // user ID of the file's owner
 unsigned int  st_gid;    // group ID of the file's group
 unsigned int  __pad0;
 SSIZE_T     st_rdev;
 SSIZE_T     st_size;     // file size, in bytes
 SSIZE_T     st_blksize;  // optimal blocksize for I/O
 SSIZE_T     st_blocks;   // Number 512-byte blocks allocated for file

 SIZE_T    st_atime;      // time of last access
 SIZE_T    st_atime_nsec;
 SIZE_T    st_mtime;      // time of last data modification
 SIZE_T    st_mtime_nsec;
 SIZE_T    st_ctime;      // time of last file status change
 SIZE_T    st_ctime_nsec;
 SSIZE_T   __unused[3];
};

static int PXCALL stat(PCHAR path, SFStat* buf);    // stat64 for x32 only
static int PXCALL fstat(int fildes, SFStat* buf);   // fstat64 for x32 only

enum EMProt
{
 PROT_NONE  = 0x00,    // Page can not be accessed.
 PROT_READ  = 0x01,    // Page can be read.
 PROT_WRITE = 0x02,    // Page can be written.
 PROT_EXEC  = 0x04,    // Page can be executed.
};

// Changes the access protections for the calling process's memory pages containing any part of the address range in the interval [addr, addr+len-1].  addr must be aligned to a page boundary.
// On success, mprotect() and pkey_mprotect() return zero.  On error, these system calls return -1, and errno is set to indicate the error.
static int PXCALL mprotect(PVOID addr, SIZE_T len, int prot);

enum EMapFlg
{
 MAP_TYPE       = 0x0f,                // Mask for type of mapping.

 MAP_SHARED     = 0x01,                // Share changes.
 MAP_PRIVATE    = 0x02,                // Changes are private.

 MAP_FIXED      = 0x10,                // Interpret addr exactly.
 MAP_ANONYMOUS  = 0x20,                // Don't use a file.
 MAP_32BIT      = 0x40,                // Only give out 32-bit addresses.


 MAP_GROWSDOWN   = 0x00100,                // Stack-like segment.
 MAP_DENYWRITE   = 0x00800,                // ETXTBSY
 MAP_EXECUTABLE  = 0x01000,                // Mark it as an executable.
 MAP_LOCKED      = 0x02000,                // Lock the mapping.
 MAP_NORESERVE   = 0x04000,                // Don't check for reservations.
 MAP_POPULATE    = 0x08000,                // Populate (prefault) pagetables.
 MAP_NONBLOCK    = 0x10000,                // Do not block on IO.
 MAP_STACK       = 0x20000,                // Allocation is for a stack.
 MAP_HUGETLB     = 0x40000,                // arch specific
 MAP_SYNC        = 0x80000,                // perform synchronous page faults for the mapping

 MAP_UNINITIALIZED = 0x4000000,
};

// Provides the same interface as mmap, except that the final argument specifies the offset into the file in 4096-byte units
static PVOID PXCALL mmap2(PVOID addr, SIZE_T length, int prot, int flags, int fd, SIZE_T pgoffset);	  // This system call does not exist on x86-64 and ARM64

// Creates a new mapping in the virtual address space of the calling process.
// Offset must be a multiple of the page size
// Check a returned addr as ((size_t)addr & 0xFFF) and if it is non zero then we have an error code which we red as -((ssize_t)addr)
static PVOID PXCALL mmap(PVOID addr, SIZE_T length, int prot, int flags, int fd, SIZE_T offset);     // Last 4 args are actually int32 (on x64 too!)	 // Since kernel 2.4 glibc mmap() invokes mmap2 with an adjusted value for offset

// Deletes the mappings for the specified address range, and causes further references to addresses within the range to generate invalid memory references.
// The address addr must be a multiple of the page size (but length need not be).
static int   PXCALL munmap(PVOID addr, SIZE_T length);

enum EMadv
{
 MADV_NORMAL    =      0,        // No further special treatment.
 MADV_RANDOM     =     1,        // Expect random page references.
 MADV_SEQUENTIAL = 2,        // Expect sequential page references.
 MADV_WILLNEED      =    3,        // Will need these pages.
 MADV_DONTNEED   =       4,        // Don't need these pages.
 MADV_FREE       =   8,        // Free pages only if memory pressure.
// Linux-cpecific
 MADV_REMOVE       =   9,        // Remove these pages and resources.
 MADV_DONTFORK      =    10,        // Do not inherit across fork.
 MADV_DOFORK        =  11,        // Do inherit across fork.
 MADV_MERGEABLE       =   12,        // KSM may merge identical pages.
 MADV_UNMERGEABLE = 13,        // KSM may not merge identical pages.
 MADV_HUGEPAGE       =   14,        // Worth backing with hugepages.
 MADV_NOHUGEPAGE  = 15,        // Not worth backing with hugepages.
 MADV_DONTDUMP       =   16,    // Explicity exclude from the core dump, overrides the coredump filter bits.
 MADV_DODUMP        =  17,        // Clear the MADV_DONTDUMP flag.
 MADV_WIPEONFORK = 18,        // Zero memory on fork, child only.
 MADV_KEEPONFORK = 19,        // Undo MADV_WIPEONFORK.
 MADV_HWPOISON       =   100,        // Poison a page for testing.
};

// Used to give advice or directions to the kernel about the address range beginning at address addr and with size length bytes In most cases, the goal of such advice is to improve system or application performance.
static int PXCALL madvise(PVOID addr, SIZE_T length, int advice);


};

using PX = NPOSIX<uint>;
//============================================================================================================


