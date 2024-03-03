
#pragma once

//============================================================================================================
// https://docs.oracle.com/cd/E19048-01/chorus5/806-6897/auto1/index.html
// https://docs.oracle.com/cd/E19048-01/chorus4/806-3328/6jcg1bm05/index.html
// https://man7.org/linux/man-pages/man2/syscalls.2.html
// https://filippo.io/linux-syscall-table/
// https://marcin.juszkiewicz.com.pl/download/tables/syscalls.html

// Only most useful POSIX functions will go here for now
// NOTE: We should be able to use these definitions to call X64 functions from X32 code if necessary (Windows only
// TODO: size of int is platform dependant, replace it
// PHS is size of pointer we require (set it default to sizeof(void*) here?))

#define PXERR(err) (-NPTM::PX::err)
#define MMERR(addr) (((size_t)addr & 0xFFF))

template<typename PHT> struct NPOSIX  // For members: alignas(sizeof(PHT))
{
// Which Linux?
template<uint vLinux, uint BSD_MAC> struct DCV   // Can be used for Kernel too
{
 static constexpr int
#if defined(SYS_MACOS) || defined(SYS_BSD)
 V = (int)BSD_MAC
#else       // Linux and Windows(emulation)
 V = (int)vLinux
#endif
;
};

template<uint vARM, uint vX86> struct ASV
{
 static constexpr int
#  if defined(CPU_ARM)
 V = (int)vARM
#else
 V = (int)vX86
#endif
;
};



 using PVOID    = SPTR<void,   PHT>;    // All of this is to be able to call X64 syscalls from X32 code
 using PCHAR    = SPTR<achar,  PHT>;  //SPTR<pchar,  PHT>;
 using PCVOID   = SPTR<const void,   PHT>;
 using PCCHAR   = SPTR<const achar,  PHT>;  //SPTR<const pchar,  PHT>;
 using PPCHAR   = SPTR<const achar*, PHT>;  //SPTR<const pchar*, PHT>;    // achar**
//using HANDLE   = PVOID;
 using SIZE_T   = decltype(TypeToUnsigned<PHT>());  //  SPTR<uint,   PHT>;
 using SSIZE_T  = decltype(TypeToSigned<PHT>());  //  SPTR<sint,   PHT>;

//using LONG     = int32;
//using ULONG    = uint32;
//using PSIZE_T  = SIZE_T*;
//using PULONG   = ULONG*;
//using NTSTATUS = LONG;
 using PSSIZE_T = SPTR<SSIZE_T, PHT>;
 using PSIZE_T  = SPTR<SIZE_T, PHT>;
 using PUINT32  = SPTR<uint32, PHT>;
 using PUINT64  = SPTR<uint64, PHT>;
 using PINT32   = SPTR<int32, PHT>;
 using PINT64   = SPTR<int64, PHT>;
 using PUINT8   = SPTR<uint8, PHT>;
 using mode_t   = int32;  //uint32;
 using fdsc_t   = int32;
 using dev_t    = uint32;    // See makedev macro
 //using off_t    = int64;
 using pid_t    = SSIZE_T;   // Should be of size_t size to contain extra info on x64 if needed
 //using fd_t     = int;
 using time_t   = SSIZE_T;   // Old time_t which is 32-bit on x32 platforms

SCVR int EOF    = -1;
SCVR int BadFD  = -1;

static constexpr bool _finline IsBadFD(fdsc_t fd){return fd < 0;}

enum EDFD  // These are just for convenience. These descriptors don`t have to be open on every system (Android?)
{
 STDIN,
 STDOUT,
 STDERR
// TODO: Add descriptors for NULL and RAND, for the framework spesifically?
};

enum EErrs   // Linux
{
 NOERROR         = 0  ,
 EPERM           = 1  , // Operation not permitted
 ENOENT          = 2  , // No such file or directory
 ESRCH           = 3  , // No such process
 EINTR           = 4  , // Interrupted system call
 EIO             = 5  , // I/O error
 ENXIO           = 6  , // No such device or address
 E2BIG           = 7  , // Arg list too long
 ENOEXEC         = 8  , // Exec format error
 EBADF           = 9  , // Bad file number
 ECHILD          = 10 , // No child processes
 EAGAIN          = 11 , // Try again
 ENOMEM          = 12 , // Out of memory
 EACCES          = 13 , // Permission denied
 EFAULT          = 14 , // Bad address
 ENOTBLK         = 15 , // Block device required
 EBUSY           = 16 , // Device or resource busy
 EEXIST          = 17 , // File exists
 EXDEV           = 18 , // Cross-device link
 ENODEV          = 19 , // No such device
 ENOTDIR         = 20 , // Not a directory
 EISDIR          = 21 , // Is a directory
 EINVAL          = 22 , // Invalid argument
 ENFILE          = 23 , // File table overflow
 EMFILE          = 24 , // Too many open files
 ENOTTY          = 25 , // Not a typewriter
 ETXTBSY         = 26 , // Text file busy
 EFBIG           = 27 , // File too large
 ENOSPC          = 28 , // No space left on device
 ESPIPE          = 29 , // Illegal seek
 EROFS           = 30 , // Read-only file system
 EMLINK          = 31 , // Too many links
 EPIPE           = 32 , // Broken pipe
 EDOM            = 33 , // Math argument out of domain of func
 ERANGE          = 34 , // Math result not representable
 EDEADLK         = 35 , // Resource deadlock would occur
 ENAMETOOLONG    = 36 , // File name too long
 ENOLCK          = 37 , // No record locks available
 ENOSYS          = 38 , // Function not implemented
 ENOTEMPTY       = 39 , // Directory not empty
 ELOOP           = 40 , // Too many symbolic links encountered
 EWOULDBLOCK     = 41 , // Operation would block
 ENOMSG          = 42 , // No message of desired type
 EIDRM           = 43 , // Identifier removed
 ECHRNG          = 44 , // Channel number out of range
 EL2NSYNC        = 45 , // Level 2 not synchronized
 EL3HLT          = 46 , // Level 3 halted
 EL3RST          = 47 , // Level 3 reset
 ELNRNG          = 48 , // Link number out of range
 EUNATCH         = 49 , // Protocol driver not attached
 ENOCSI          = 50 , // No CSI structure available
 EL2HLT          = 51 , // Level 2 halted
 EBADE           = 52 , // Invalid exchange
 EBADR           = 53 , // Invalid request descriptor
 EXFULL          = 54 , // Exchange full
 ENOANO          = 55 , // No anode
 EBADRQC         = 56 , // Invalid request code
 EBADSLT         = 57 , // Invalid slot
 EDEADLOCK       = 58 , // File locking deadlock error
 EBFONT          = 59 , // Bad font file format
 ENOSTR          = 60 , // Device not a stream
 ENODATA         = 61 , // No data available
 ETIME           = 62 , // Timer expired
 ENOSR           = 63 , // Out of streams resources
 ENONET          = 64 , // Machine is not on the network
 ENOPKG          = 65 , // Package not installed
 EREMOTE         = 66 , // Object is remote
 ENOLINK         = 67 , // Link has been severed
 EADV            = 68 , // Advertise error
 ESRMNT          = 69 , // Srmount error
 ECOMM           = 70 , // Communication error on send
 EPROTO          = 71 , // Protocol error
 EMULTIHOP       = 72 , // Multihop attempted
 EDOTDOT         = 73 , // RFS specific error
 EBADMSG         = 74 , // Not a data message
 EOVERFLOW       = 75 , // Value too large for defined data type
 ENOTUNIQ        = 76 , // Name not unique on network
 EBADFD          = 77 , // File descriptor in bad state
 EREMCHG         = 78 , // Remote address changed
 ELIBACC         = 79 , // Can not access a needed shared library
 ELIBBAD         = 80 , // Accessing a corrupted shared library
 ELIBSCN         = 81 , // .lib section in a.out corrupted
 ELIBMAX         = 82 , // Attempting to link in too many shared libraries
 ELIBEXEC        = 83 , // Cannot exec a shared library directly
 EILSEQ          = 84 , // Illegal byte sequence
 ERESTART        = 85 , // Interrupted system call should be restarted
 ESTRPIPE        = 86 , // Streams pipe error
 EUSERS          = 87 , // Too many users
 ENOTSOCK        = 88 , // Socket operation on non-socket
 EDESTADDRREQ    = 89 , // Destination address required
 EMSGSIZE        = 90 , // Message too long
 EPROTOTYPE      = 91 , // Protocol wrong type for socket
 ENOPROTOOPT     = 92 , // Protocol not available
 EPROTONOSUPPORT = 93 , // Protocol not supported
 ESOCKTNOSUPPORT = 94 , // Socket type not supported
 EOPNOTSUPP      = 95 , // Operation not supported on transport endpoint
 EPFNOSUPPORT    = 96 , // Protocol family not supported
 EAFNOSUPPORT    = 97 , // Address family not supported by protocol
 EADDRINUSE      = 98 , // Address already in use
 EADDRNOTAVAIL   = 99 , // Cannot assign requested address
 ENETDOWN        = 100, // Network is down
 ENETUNREACH     = 101, // Network is unreachable
 ENETRESET       = 102, // Network dropped connection because of reset
 ECONNABORTED    = 103, // Software caused connection abort
 ECONNRESET      = 104, // Connection reset by peer
 ENOBUFS         = 105, // No buffer space available
 EISCONN         = 106, // Transport endpoint is already connected
 ENOTCONN        = 107, // Transport endpoint is not connected
 ESHUTDOWN       = 108, // Cannot send after transport endpoint shutdown
 ETOOMANYREFS    = 109, // Too many references: cannot splice
 ETIMEDOUT       = 110, // Connection timed out
 ECONNREFUSED    = 111, // Connection refused
 EHOSTDOWN       = 112, // Host is down
 EHOSTUNREACH    = 113, // No route to host
 EALREADY        = 114, // Operation already in progress
 EINPROGRESS     = 115, // Operation now in progress
 ESTALE          = 116, // Stale NFS file handle
 EUCLEAN         = 117, // Structure needs cleaning
 ENOTNAM         = 118, // Not a XENIX named type file
 ENAVAIL         = 119, // No XENIX semaphores available
 EISNAM          = 120, // Is a named type file
 EREMOTEIO       = 121, // Remote I/O error
};

// https://android.googlesource.com/platform/bionic/+/master/libc/include/bits/signal_types.h
enum ESignals
{
 SIGHUP    = 1,
 SIGINT    = 2,
 SIGQUIT   = 3,
 SIGILL    = 4,
 SIGTRAP   = 5,
 SIGABRT   = 6,
 SIGIOT    = 6,
 SIGBUS    = 7,
 SIGFPE    = 8,
 SIGKILL   = 9,
 SIGUSR1   = 10,
 SIGSEGV   = 11,
 SIGUSR2   = 12,
 SIGPIPE   = 13,
 SIGALRM   = 14,
 SIGTERM   = 15,
 SIGSTKFLT = 16,
 SIGCHLD   = 17,
 SIGCONT   = 18,
 SIGSTOP   = 19,
 SIGTSTP   = 20,
 SIGTTIN   = 21,
 SIGTTOU   = 22,
 SIGURG    = 23,
 SIGXCPU   = 24,
 SIGXFSZ   = 25,
 SIGVTALRM = 26,
 SIGPROF   = 27,
 SIGWINCH  = 28,
 SIGIO     = 29,
 SIGPOLL   = SIGIO,
 SIGPWR    = 30,
 SIGSYS    = 31,
 SIGUNUSED = 31
};

// =========================================== FILE/DIRECTORY ===========================================
// http://cqctworld.org/src/l1/lib/linux-x86-enum.names
// https://man7.org/linux/man-pages/man2/open.2.html
// https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2uapi_2asm-generic_2fcntl_8h.html
// https://opensource.apple.com/source/xnu/xnu-344/bsd/sys/fcntl.h
// https://github.com/dlang/druntime/blob/master/src/core/sys/posix/fcntl.d

// MacOS: xnu-2422.1.72\bsd\dev\dtrace\scripts\io.d

// Extended/sys-unsupported (for the Framework) flags are put in the high byte
enum EOpnFlg    // For 'flags' field    // Platform/Arch dependant?
{               //      LINUX       BSD/MacOS
  O_ACCMODE   = 0x00000003,    // Mask to test one of access modes: if((mode & O_ACCMODE) == O_RDONLY)
  O_RDONLY    = 0x00000000,    // Open for reading only.
  O_WRONLY    = 0x00000001,    // Open for writing only.
  O_RDWR      = 0x00000002,    // Open for reading and writing.

//O_SHLOCK    = 0x00000010,    // BSD/MacOS ???
//O_EXLOCK    = 0x00000020,    // BSD/MacOS ???
  O_ASYNC     = DCV< 0         , 0x00000040 >::V,    // Sends SIGIO or SIGPOLL
  O_SYMLINK   = DCV< 0x80000000, 0x00200000 >::V,    // WinNT+: BSD/MacOS: allow open of symlinks: if the target file passed to open() is a symbolic link then the open() will be for the symbolic link itself, not what it links to.

  O_CREAT     = DCV< 0x00000040, 0x00000200 >::V,    // If the file exists, this flag has no effect. Otherwise, the owner ID of the file is set to the user ID of the c_actor, the group ID of the file is set to the group ID of the c_actor, and the low-order 12 bits of the file mode are set to the value of mode.
  O_EXCL      = DCV< 0x00000080, 0x00000800 >::V,    // Ensure that this call creates the file. If O_EXCL and O_CREAT are set, open will fail if the file exists. In general, the behavior of O_EXCL is undefined if it is used without O_CREAT

  O_TRUNC     = DCV< 0x00000200, 0x00000400 >::V,    // If the file exists, its length is truncated to 0 and the mode and owner are unchanged.
  O_APPEND    = DCV< 0x00000400, 0x00000008 >::V,    // If set, the file pointer will be set to the end of the file prior to each write.
  O_NONBLOCK  = DCV< 0x00000800, 0x00000004 >::V,    // If O_NONBLOCK is set, the open will return without waiting for the device to be ready or available. Subsequent behavior of the device is device-specific.
 // Additional:
  O_DSYNC     = DCV< 0x00001000, 0          >::V,
  O_DIRECT    = DCV< ASV<0x00010000,0x00004000>::V, 0          >::V,    // direct disk access hint - currently ignored
  O_LARGEFILE = DCV< ASV<0x00020000,0x00008000>::V, 0          >::V,
  O_DIRECTORY = DCV< ASV<0x00004000,0x00010000>::V, 0x00100000 >::V,    // must be a directory  // This flag is unreliable: X86=0x00010000, ARM=0x00004000 (Probably)    // NOTE: Required to open directories on Windows
  O_NOFOLLOW  = DCV< ASV<0x00008000,0x00020000>::V, 0x00000100 >::V,    // don't follow links: if the target file passed to open() is a symbolic link then the open() will fail
  O_NOATIME   = DCV< 0x00040000, 0          >::V,
  O_CLOEXEC   = DCV< 0x00080000, 0x01000000 >::V,    // set close_on_exec    // DARWIN LEVEL >= 200809
  O_PATH      = DCV< 0x00200000, 0          >::V,
  O_TMPFILE   = DCV< 0x00410000, 0          >::V,    // Create an unnamed temporary regular file. The pathname argument specifies a directory; an unnamed inode will be created in that directory's filesystem.
  O_SYNC      = DCV< 0x00101000, 0x00000080 >::V,    // By the time write(2) (or similar) returns, the output data and associated file metadata have been transferred to the underlying hardware (i.e., as though each write(2) was followed by a call to fsync(2))
};

// https://github.com/torvalds/linux/blob/master/include/uapi/linux/stat.h
enum ENode     // for mknod
{
 S_IFMT   = 0x0000F000, // 00170000
 S_IFSOCK = 0x0000C000, // 0140000
 S_IFLNK  = 0x0000A000, // 0120000    // Link
 S_IFREG  = 0x00008000, // 0100000    // Regular file
 S_IFBLK  = 0x00006000, // 0060000
 S_IFDIR  = 0x00004000, // 0040000    // Directory
 S_IFCHR  = 0x00002000, // 0020000
 S_IFIFO  = 0x00001000, // 0010000
// S_ISUID  = 0x00000800, // 0004000
// S_ISGID  = 0x00000400, // 0002000
// S_ISVTX  = 0x00000200, // 0001000

//#define S_ISLNK(m)    (((m) & S_IFMT) == S_IFLNK)
//#define S_ISREG(m)    (((m) & S_IFMT) == S_IFREG)
//#define S_ISDIR(m)    (((m) & S_IFMT) == S_IFDIR)
//#define S_ISCHR(m)    (((m) & S_IFMT) == S_IFCHR)
//#define S_ISBLK(m)    (((m) & S_IFMT) == S_IFBLK)
//#define S_ISFIFO(m)   (((m) & S_IFMT) == S_IFIFO)
//#define S_ISSOCK(m)   (((m) & S_IFMT) == S_IFSOCK)
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

// The path parameter points to a path name naming a file. The open function opens a file descriptor for the named file and sets the file status flags according to the value of oflag.
static fdsc_t PXCALL open(PCCHAR pathname, int flags, mode_t mode);  // 'open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode)' is same as call to 'creat'

// Equivalent to: open(path, O_WRONLY|O_CREAT|O_TRUNC, mode)
//static int PXCALL creat(PCCHAR path, mode_t mode);    // Use it as fallback if a correct O_CREAT cannot be found

// The fildes field contains a file descriptor obtained from an open(2POSIX), dup(2POSIX), accept(2POSIX), socket(2POSIX), or shm_open(2POSIX) system call. The close function closes the file descriptor indicated by fildes.
static int PXCALL close(fdsc_t fd);


struct iovec
{
 PVOID  iov_base;   // base address
 SIZE_T iov_len;    // length
};

using PIOVec = SPTR<iovec,   PHT>;

// Attempts to read nbytes of data from the object referenced by the descriptor d into the buffer pointed to by buf . readv() performs the same action, but scatters the input data into the iovcnt buffers specified by the members of the iov array: iov[0] , iov[1] , ..., iov[iovcnt-1] .
// Upon successful completion, read() and readv() return the number of bytes actually read and placed in the buffer. The system guarantees to read the number of bytes requested if the descriptor references a normal file that contains that many bytes before the end-of-file, but in no other case. Upon end-of-file, 0 is returned. read() and readv() also return 0 if a non-blocking read is attempted on a socket that is no longer open. Otherwise, -1 is returned, and the global variable errno is set to indicate the error.
// return 0 is when the size argument was 0 or end-of-file has been reached
// for pipes, end-of-file means the writing end of the pipe has been closed
static SSIZE_T PXCALL read(fdsc_t fd, PVOID buf, SIZE_T nbytes);
static SSIZE_T PXCALL readv(fdsc_t fd, PIOVec iov, int iovcnt);

// Attempts to write nbytes of data to the object referenced by the descriptor d from the buffer pointed to by buf . writev() performs the same action, but gathers the output data from the iovcnt buffers specified by the members of the iov array: iov[0] , iov[1] , ..., iov[iovcnt-1] .
// Upon successful completion, write() and writev() return the number of bytes actually written. Otherwise, they return -1 and set errno to indicate the error.
static SSIZE_T PXCALL write(fdsc_t fd, PCVOID buf, SIZE_T nbytes);
static SSIZE_T PXCALL writev(fdsc_t fd, PIOVec iov, int iovcnt);     // Windows: WriteFileGather

enum ESeek
{
 SEEK_SET  = 0,       // Seek relative to begining of file
 SEEK_CUR  = 1,       // Seek relative to current file position
 SEEK_END  = 2,       // Seek relative to end of file

 // Linux 3.1: SEEK_DATA, SEEK_HOLE
};

// Repositions the file offset of the open file description associated with the file descriptor fd to the argument offset according to the directive whence
// Negative return values are error codes
static SSIZE_T PXCALL lseek(fdsc_t fd, SSIZE_T offset, ESeek whence);   // This definition is not good for X32, use INT64 (lseekGD) declaration and llseek wrapper on X32

static int64 PXCALL lseekGD(fdsc_t fd, int64 offset, ESeek whence);   // Generic definition, wraps lseek(on x64) and llseek(on x32)

// x32 only(Not present on x64)!
static int PXCALL llseek(fdsc_t fd, uint32 offset_high, uint32 offset_low, PINT64 result, ESeek whence);

// Attempts to create a directory named pathname.
static int PXCALL mkdir(PCCHAR pathname, mode_t mode);

// Deletes a directory, which must be empty
static int PXCALL rmdir(PCCHAR pathname);

// Renames a file, moving it between directories if required.  Any other hard links to the file (as created using link(2)) are unaffected.  Open file descriptors for oldpath are also unaffected.
static int PXCALL rename(PCCHAR oldpath, PCCHAR newpath);

// Places the contents of the SYMBOLIC link pathname in the buffer buf, which has size bufsiz.  readlink() does not append a terminating null byte to buf.  It will (silently) truncate the contents (to a length of bufsiz characters), in case the buffer is too small to hold all of the contents.
// On success return the number of bytes placed in buf. (If the returned value equals bufsiz, then truncation may have occurred.)
static SSIZE_T PXCALL readlink(PCCHAR pathname, PCHAR buf, SIZE_T bufsize);

// Deletes a name from the filesystem.  If that name was the last link to a file and no processes have the file open, the file is deleted and the space it was using is made available for reuse.
// If the name was the last link to a file but any processes still have the file open, the file will remain in existence until the last file descriptor referring to it is closed.
// If the name referred to a SYMBOLIC link, the link is removed.
// If the name referred to a socket, FIFO, or device, the name for it is removed but processes which have the object open may continue to use it.
static int PXCALL unlink(PCCHAR pathname);

// creates a new link (also known as a hard link (INODE on current FS)) to an existing file.
// File`s Hard links all refer to same data (identified by INODE) so any of them can be moved freely within the FS
// The file itself exist while at least one Hard Link to its INODE exists and there is open descriptors to it
// since kernel 2.0, Linux does not do so: if oldpath is a symbolic link, then newpath is created as a (hard) link to the same symbolic link file (i.e., newpath becomes a symbolic link to the same file that oldpath refers to).
static int PXCALL link(PCCHAR oldpath, PCCHAR newpath);

// creates a SYMBOLIC link named linkpath which contains the string target.
// Symbolic links are interpreted at run time as if the contents of the link had been substituted into the path being followed tofind a file or directory.
// Symbolic links may contain ..  path components, which (if used at the start of the link) refer to the parent directories of that in which the link resides.
// A symbolic link (also known as a soft link) may point to an existing file or to a nonexistent one; the latter case is known as a dangling link.
static int PXCALL symlink(PCCHAR target, PCCHAR linkpath);

enum EAcss
{
 F_OK   =   0,      // test for existence of file
 X_OK   =   0x01,   // test for execute or search permission
 W_OK   =   0x02,   // test for write permission
 R_OK   =   0x04,   // test for read permission
};

// Checks whether the calling process can access the file pathname.  If pathname is a symbolic link, it is dereferenced.
static int PXCALL access(PCCHAR pathname, int mode);

// Creates a filesystem node (file, device special file, or named pipe) named pathname, with attributes specified by mode and dev.
static int PXCALL mknod(PCCHAR pathname, mode_t mode, dev_t dev);

// Create a named pipe
static int PXCALL mkfifo(PCCHAR pathname, mode_t mode);  // Not a syscall on linux and on BSD is just a wrapper for mknod

// Create an unnamed pipe
// pipefd[0] refers to the read end of the pipe.  pipefd[1] refers to the write end of the pipe.
// NOTE: reading the read end of a pipe will return EOF when and only when all copies of the write end of the pipe are closed.
static int PXCALL pipe(PINT32 fds);
static int PXCALL pipe2(PINT32 pipefd, int flags);   // int pipefd[2]    // Is Linux-specific? On BSD since V10 (2014)

// Uses the lowest-numbered unused descriptor for the new descriptor
static int PXCALL dup(int oldfd);

// Makes newfd be the copy of oldfd, closing newfd first if necessary
// If oldfd is not a valid file descriptor, then the call fails, and newfd is not closed
// If oldfd is a valid file descriptor, and newfd has the same value as oldfd, then dup2() does nothing, and returns newfd.
static int PXCALL dup3(int oldfd, int newfd, int flags);

enum EFcntl
{
 F_DUPFD         = 0,        // dup
 F_GETFD         = 1,        // get close_on_exec
 F_SETFD         = 2,        // set/clear close_on_exec
 F_GETFL         = 3,        // get file->f_flags
 F_SETFL         = 4,        // set file->f_flags

 F_GETLK         = 5,
 F_SETLK         = 6,
 F_SETLKW        = 7,

 F_SETOWN        = 8,        // for sockets
 F_GETOWN        = 9,        // for sockets

 F_SETSIG        = 10,       // for sockets
 F_GETSIG        = 11,       // for sockets

 F_NOTIFY          = 0x402,    // (F_LINUX_SPECIFIC_BASE + 2)
 F_DUPFD_CLOEXEC   = 0x406     // (F_LINUX_SPECIFIC_BASE + 6),  // F_LINUX_SPECIFIC_BASE = 1024,
};

enum EDNotify  // Types of directory notifications that may be requested with fcntl
{
 DN_ACCESS     = 0x00000001,   // File accessed
 DN_MODIFY     = 0x00000002,   // File modified
 DN_CREATE     = 0x00000004,   // File created
 DN_DELETE     = 0x00000008,   // File removed
 DN_RENAME     = 0x00000010,   // File renamed
 DN_ATTRIB     = 0x00000020,   // File changed attibutes
 DN_MULTISHOT  = 0x80000000    // Don't remove notifier
};

static int PXCALL fcntl(fdsc_t fd, int cmd, SIZE_T arg);  // for a directory change notification (lagacy, Linux only) : https://linux.die.net/man/7/inotify

enum EFDLock
{
 LOCK_SH    = 1,    // Shared lock
 LOCK_EX    = 2,    // Exclusive lock
 LOCK_NB    = 4,    // Or'd with one of the above to prevent blocking
 LOCK_UN    = 8  // Remove lock
};

static int PXCALL fsync(fdsc_t fd);

static int PXCALL fdatasync(fdsc_t fd);

static int PXCALL flock(fdsc_t fd, int operation);


// https://en.cppreference.com/w/c/chrono/timespec
template<typename T> struct STSpec        // nanosleep, fstat (SFStat)
{
 T sec;   // Seconds      // time_t ???
 T nsec;  // Nanoseconds  // valid values are [0, 999999999]   // long (long ong on some platforms?)

inline STSpec<T>& operator= (const auto& tm){this->sec = (T)tm.sec; this->nsec = (T)tm.nsec; return *this;}
};
using timespec = STSpec<time_t>;
using PTiSp = SPTR<timespec,   PHT>;

template<typename T> struct STVal      // gettimeofday
{
 T sec;   // Seconds         // time_t (long)
 T usec;  // Microseconds    // suseconds_t (long)
};
using timeval = STVal<time_t>;
using PTiVl = SPTR<timeval,   PHT>;

struct timezone
{
 sint32 utcoffs;     // minutes west of Greenwich  // Seconds now (to avoid multiplication when modifying UTC time in seconds)
 sint32 dsttime;     // type of DST correction     // Unused
};

static int PXCALL gettimeofday(timeval* tv, timezone* tz);   // Returns 0 in timezone on Linux
static int PXCALL settimeofday(timeval* tv, timezone* tz);

struct pollfd
{
 int   fd;         // file descriptor
 short events;     // requested events
 short revents;    // returned events
};

struct fd_set     // Each bit represents a triggered file descriptor. Total bits is max file descriptors range which is passed in nfds
{
 static const int BitsPerElem = (sizeof(size_t)*8);
 size_t fds_bits[1024 / BitsPerElem];  // long x64=64,x32=32 // FD_SET // Max possible file descriptors is 4096(Hard limit) and max 1024 per process (Soft limit)
};
// typedef size_t fd_set[1024 / sizeof(size_t)];

using nfds_t = uint32;
static int PXCALL poll(pollfd* fds, nfds_t nfds, int timeout);   // Volatile (Flags only?)


// https://unix.stackexchange.com/questions/84227/limits-on-the-number-of-file-descriptors
// https://stackoverflow.com/questions/18952564/understanding-fd-set-in-unix-sys-select-h
//static int PXCALL select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval* timeout);    // Volatile and complicated   // Ignore it completely, poll is just faster because there is no need to parse bitfields

// /arch/{ARCH}/include/uapi/asm/stat.h
// https://stackoverflow.com/questions/29249736/what-is-the-precise-definition-of-the-structure-passed-to-the-stat-system-call
// Since kernel 2.5.48, the stat structure supports nanosecond resolution for the three file timestamp fields.
// Too volatile to use crossplatform?

struct SFStat      // Generic     // This one is used in NAPI
{
 uint64 dev;            // ID of device containing file
 uint64 ino;            // inode number
 uint32 nlink;          // number of hard links
 uint32 mode;           // protection and type
 uint32 uid;            // user ID of owner
 uint32 gid;            // group ID of owner
 uint64 rdev;           // device ID (if special file)
 sint64 size;           // total size, in bytes
 uint64 blksize;        // blocksize for file system I/O
 uint64 blocks;         // Number 512-byte blocks allocated.
 STSpec<uint64> atime;  // The field st_atime is changed by file accesses, for example, by execve(2), mknod(2), pipe(2), utime(2) and read(2) (of more than zero bytes)
 STSpec<uint64> mtime;  // The field st_mtime is changed by file modifications, for example, by mknod(2), truncate(2), utime(2) and write(2) (of more than zero bytes)
 STSpec<uint64> ctime;  // The field st_ctime is changed by writing or by setting inode information (i.e., owner, group, link count, mode, etc.)
};

struct SFStatX86x64      // On x86_64
{
 uint64 dev;
 uint64 ino;
 uint64 nlink;
 uint32 mode;
 uint32 uid;
 uint32 gid;
 uint32 __pad0;
 uint64 rdev;
 sint64 size;
 uint64 blksize;
 uint64 blocks;
 STSpec<uint64> atime;
 STSpec<uint64> mtime;
 STSpec<uint64> ctime;
 int64  __unused[3];
};

/*struct SFStatX86x32
{

};*/

struct SFStatArm64      // On raspberry pi ARMx64
{
 uint64 dev;
 uint64 ino;
 uint32 mode;
 uint32 nlink;
 uint64 uid;
 uint64 gid;
 uint64 rdev;
 sint64 size;
 uint64 blksize;
 uint64 blocks;
 STSpec<uint64> atime;
 STSpec<uint64> mtime;
 STSpec<uint64> ctime;
 int64  __unused[3];
};

/*struct SFStatArm32     // Do not use - 32bit sizes! (Use SFStat64 on x32 systems with xxx64 functions instead)
{
 uint32 dev;
 uint32 ino;
 uint16 mode;
 uint16 nlink;
 uint16 uid;
 uint16 gid;
 uint32 rdev;
 uint32 size;
 uint32 blksize;
 uint32 blocks;
 STSpec<uint32> atime;
 STSpec<uint32> mtime;
 STSpec<uint32> ctime;
 uint32  unused[2]
}; */

struct SFStat64    // On ARMx32,...  For xxx64 functions (mode,nlink,uid,gid,size is compatible with SFStatArm64)
{
 uint64  dev;
 uint8   __pad0[4];
 uint32  __ino;       // inode number
 uint32  mode;
 uint32  nlink;
 uint32  uid;
 uint32  gid;
 uint64  rdev;
 uint8   __pad3[4];
 sint64  size;
 uint32  blksize;
 uint64  blocks;      // Number 512-byte blocks allocated.
 STSpec<uint32>  atime;       // time of last access
 STSpec<uint32>  mtime;       // time of last modification
 STSpec<uint32>  ctime;       // time of last status change
 uint64  ino;
};

static void ConvertToNormalFstat(SFStat* Dst, vptr Src)
{
 using TSrcType = typename TSW<IsArchX64, typename TSW<IsCpuARM, SFStatArm64, SFStatX86x64>::T, typename TSW<IsCpuARM, SFStat64, SFStat64>::T>::T;   // TODO: BSD, XNU    // Is SFStat64 same on ARM and X86?
 TSrcType* SrcPtr = (TSrcType*)Src;
 SFStat Tmp;   // Dst may be same as Src

 Tmp.dev     = SrcPtr->dev;
 Tmp.ino     = SrcPtr->ino;
 Tmp.nlink   = SrcPtr->nlink;
 Tmp.mode    = SrcPtr->mode;
 Tmp.uid     = SrcPtr->uid;
 Tmp.gid     = SrcPtr->gid;
 Tmp.rdev    = SrcPtr->rdev;
 Tmp.size    = SrcPtr->size;
 Tmp.blksize = SrcPtr->blksize;
 Tmp.blocks  = SrcPtr->blocks;
 Tmp.atime   = SrcPtr->atime;   // May be STSpec<uint32>  to  STSpec<uint64>
 Tmp.mtime   = SrcPtr->mtime;
 Tmp.ctime   = SrcPtr->ctime;

 Dst->dev     = Tmp.dev;
 Dst->ino     = Tmp.ino;
 Dst->nlink   = Tmp.nlink;
 Dst->mode    = Tmp.mode;
 Dst->uid     = Tmp.uid;
 Dst->gid     = Tmp.gid;
 Dst->rdev    = Tmp.rdev;
 Dst->size    = Tmp.size;
 Dst->blksize = Tmp.blksize;
 Dst->blocks  = Tmp.blocks;
 Dst->atime   = Tmp.atime;
 Dst->mtime   = Tmp.mtime;
 Dst->ctime   = Tmp.ctime;
}
//------------------------------------------------------------------------------------------------------------

// st_mode:
// xxxx xxxx  xxxx xxxx  xxxx xxxOOOGGGTTT
// O - Owner
// G - Group
// T - Other
//
//
// Flags: AT_SYMLINK_NOFOLLOW  AT_RESOLVE_BENEATH  AT_EMPTY_PATH

using PFStat   = SPTR<SFStat, PHT>;
using PFStat64 = SPTR<SFStat64, PHT>;

static int PXCALL stat(PCCHAR path, PFStat buf);         // Not on Arm64      // NOTE: Do not use, returns 32bit sizes!
static int PXCALL stat64(PCCHAR path, PFStat64 buf);     // stat64 for x32 only

static int PXCALL fstat(fdsc_t fildes, PFStat buf);      // On x32 and x64
static int PXCALL fstat64(fdsc_t fildes, PFStat64 buf);  // fstat64 for x32 only

static int PXCALL fstatat(fdsc_t dirfd, PCCHAR pathname, PFStat buf, int flags);      // On x64 only (called newfstatat)
static int PXCALL fstatat64(fdsc_t dirfd, PCCHAR pathname, PFStat64 buf, int flags);  // fstatat64 for x32 only

enum EATExtra
{
 AT_FDCWD            = DCV< (uint)-100, (uint)-2 >::V,  // Special value used to indicate openat should use the current working directory.
 AT_SYMLINK_NOFOLLOW = DCV< 0x100, 0x0020 >::V,  // Do not follow symbolic links.
 AT_REMOVEDIR        = DCV< 0x200, 0x0080 >::V,  // Remove directory instead of unlinking file.
 AT_SYMLINK_FOLLOW   = DCV< 0x400, 0x0040 >::V,  // Follow symbolic links.
};


// The  d_type field is implemented since Linux 2.6.4.  It occupies a space that was previously a zero-filled padding byte in the linux_dirent structure.  Thus, on kernels up to and including 2.6.3, attempting to access this field always provides the value 0 (DT_UNKNOWN).
// Currently, only some filesystems (among them: Btrfs, ext2, ext3, and ext4) have full support for returning the file type  in  d_type.   All applications must properly handle a return of DT_UNKNOWN.
enum EDEntType
{
 DT_UNKNOWN = 0,   // The file type is unknown
 DT_FIFO    = 1,   // This is a named pipe (FIFO)
 DT_CHR     = 2,   // This is a character device
 DT_DIR     = 4,   // This is a directory
 DT_BLK     = 6,   // This is a block device
 DT_REG     = 8,   // This is a regular file
 DT_LNK     = 10,  // This is a symbolic link
 DT_SOCK    = 12,  // This is a UNIX domain socket
 DT_WHT     = 14   // BSD/Darwin

// Framework extended:   // Bad idea - on Linux will have to loop through all records after getdents to change the flags and do 'stat' on links even if none of this will be useful afterwards
/* DET_FIFO    = 0x01,
 DET_CHR     = 0x02,
 DET_DIR     = 0x04,
 DET_BLK     = 0x08,
 DET_REG     = 0x10,
 DET_LNK     = 0x20,
 DET_SOCK    = 0x40,
 DET_WHT     = 0x80 */
};

struct darwin_dirent32   // when _DARWIN_FEATURE_64_BIT_INODE is NOT defined     // Untested!
{
 uint32 ino;             // file number of entry
 uint16 reclen;          // length of this record
 uint8  type;            // file type, see below
 uint8  namlen;          // length of string in d_name
 achar  name[255 + 1];   // name must be no longer than this
};

struct darwin_dirent64   // when _DARWIN_FEATURE_64_BIT_INODE is defined         // Untested!
{
 uint64 fileno;          // file number of entry
 uint64 seekoff;         // seek offset (optional, used by servers)
 uint16 reclen;          // length of this record
 uint16 namlen;          // length of string in d_name
 uint8  type;            // file type, see below
 achar  name[1024];      // name must be no longer than this
};

struct bsd_dirent32      // For syscall 196 (freebsd11)     _WANT_FREEBSD11_DIRENT
{
 uint32 fileno;          // file number of entry
 uint16 reclen;          // length of this record
 uint8  type;            // file type, see below
 uint8  namlen;          // length of string in d_name
 achar  name[255 + 1];   // name must be no longer than this
};

struct bsd_dirent64      // For syscall 554  // BSDSysVer >= 1200031
{
 uint64 fileno;		     // file number of entry
 sint64 off;		     // directory offset of next entry
 uint16 reclen;		     // length of this record
 uint8  type;		     // file type, see below
 uint8  pad0;
 uint16 namlen;		     // length of string in d_name
 uint16 pad1;
 achar  name[255 + 1];   // name must be no longer than this
};


struct SDirEnt     // linux_dirent64  // For getdents64
{
 uint64 ino;       // Inode number // BSD: ino_t   d_fileno
 sint64 off;       // Offset to next linux_dirent   // BSD: ff_t d_off
 uint16 reclen;    // Length of this linux_dirent
 uint8  type;      // File type (only since Linux 2.6.4;
 achar  name[1];   // Filename (null-terminated)    // length is actually (d_reclen - 2 - offsetof(struct linux_dirent, d_name)
};

//using PSDirEnt = SPTR<SDirEnt, PHT>;

// Queries a directory atomically, advances the file position if the buffer too small to fit all entries
static int PXCALL getdentsGD(fdsc_t fd, PVOID buf, SIZE_T bufsize);   // General definition
static int PXCALL getdents32(fdsc_t fd, PVOID buf, SIZE_T bufsize);      // Unused
static int PXCALL getdents64(fdsc_t fd, PVOID buf, SIZE_T bufsize);      // unsigned int count

// If the basep pointer value is non-NULL, the getdirentries() system call writes the position of the block read into the location pointed to by basep.
static int PXCALL getdirentries32(fdsc_t fd, PVOID buf, SIZE_T bufsize, PSSIZE_T basep);    // BSD: 196;  XNU: 196  [freebsd11] getdirentries   // Returns SSIZE_T?
static int PXCALL getdirentries64(fdsc_t fd, PVOID buf, SIZE_T bufsize, PSSIZE_T basep);    // BSD: 554;  XNU: 344                              // Returns SSIZE_T?

// New, For Arm64
static int PXCALL openat(fdsc_t dirfd, PCCHAR pathname, int flags, mode_t mode);
static int PXCALL mknodat(fdsc_t dirfd, PCCHAR pathname, mode_t mode, dev_t dev);
static int PXCALL mkdirat(fdsc_t dirfd, PCCHAR pathname, mode_t mode);
static int PXCALL linkat(fdsc_t olddirfd, PCCHAR oldpath, fdsc_t newdirfd, PCCHAR newpath, int flags);
static int PXCALL unlinkat(fdsc_t dirfd, PCCHAR pathname, int flags);
static int PXCALL renameat(fdsc_t olddirfd, PCCHAR oldpath, fdsc_t newdirfd, PCCHAR newpath);
static int PXCALL symlinkat(PCCHAR target, fdsc_t newdirfd, PCCHAR linkpath);
static int PXCALL readlinkat(fdsc_t dirfd, PCCHAR pathname, PCHAR buf, SIZE_T bufsiz);
static int PXCALL faccessat(fdsc_t dirfd, PCCHAR pathname, int mode, int flags);

// =========================================== MEMORY ===========================================
enum EMapProt
{
 PROT_NONE  = 0x00,    // Page can not be accessed.
 PROT_READ  = 0x01,    // Page can be read.
 PROT_WRITE = 0x02,    // Page can be written.
 PROT_EXEC  = 0x04,    // Page can be executed.
};

// Changes the access protections for the calling process's memory pages containing any part of the address range in the interval [addr, addr+len-1].  addr must be aligned to a page boundary.
// On success, mprotect() and pkey_mprotect() return zero.  On error, these system calls return -1, and errno is set to indicate the error.
static int PXCALL mprotect(PVOID addr, SIZE_T len, int prot);

// https://github.com/nneonneo/osx-10.9-opensource/blob/master/xnu-2422.1.72/bsd/sys/mman.h#L150     // <<<<<<<<<<<<<<< Not match!
//
enum EMapFlg
{
 MAP_TYPE       = 0x0f,                // Mask for type of mapping.

 MAP_SHARED     = 0x01,                // Share changes.
 MAP_PRIVATE    = 0x02,                // Changes are private.

 MAP_FIXED      = 0x10,                // Interpret addr exactly.  MAP_FIXED_NOREPLACE is preferrable (since Linux 4.17)
 MAP_ANONYMOUS  = DCV< 0x20, 0x1000  >::V,                 // Don't use a file.  // BSD?
 MAP_ANON       = MAP_ANONYMOUS,       // allocated from memory, swap space
 MAP_32BIT      = DCV< 0x40, 0x8000  >::V,                 // Only give out 32-bit addresses(< 4GB). // BSD?
 MAP_FILE       = 0x00,                // map from file (default)

 MAP_GROWSDOWN  = DCV< 0x00100, 0  >::V,              // Stack-like segment.
 MAP_LOCKED     = DCV< 0x02000, 0  >::V,              // Lock the mapping.
 MAP_NORESERVE  = DCV< 0x04000, 0x0040 >::V,          // Don't check for reservations.
 MAP_POPULATE   = DCV< 0x08000, 0  >::V,              // Populate (prefault) pagetables.
 MAP_NONBLOCK   = DCV< 0x10000, 0  >::V,              // Do not block on IO.
 MAP_STACK      = DCV< 0x20000, 0  >::V,              // Allocation is for a stack.
 MAP_HUGETLB    = DCV< 0x40000, 0  >::V,              // arch specific
 MAP_SYNC       = DCV< 0x80000, 0  >::V,              // perform synchronous page faults for the mapping
 MAP_JIT        = DCV< 0,  0x0800    >::V, // MacOS only // Allocate a region that will be used for JIT purposes  // BSD?
 MAP_NOCACHE    = DCV< 0,  0x0400    >::V, // don't cache pages for this mapping

 MAP_UNINITIALIZED = DCV< 0x4000000, 0 >::V,

// MacOS: MAP_RESILIENT_MEDIA=0x4000, MAP_RESILIENT_CODESIGN=0x2000
};

// Provides the same interface as mmap, except that the final argument specifies the offset into the file in 4096-byte units
static PVOID PXCALL mmapGD(PVOID addr, SIZE_T length, uint prot, uint flags, fdsc_t fd, uint64 pgoffset);    // Generic definition for x32/x64   // On X32 value of x64 pgoffset is shifted right for mmap2

static PVOID PXCALL mmap2(PVOID addr, SIZE_T length, uint prot, uint flags, fdsc_t fd, SIZE_T pgoffset);     // This system call does not exist on x86-64 and ARM64

// Creates a new mapping in the virtual address space of the calling process.
// Offset must be a multiple of the page size
// Check a returned addr as ((size_t)addr & 0xFFF) and if it is non zero then we have an error code which we red as -((ssize_t)addr)
static PVOID PXCALL mmap(PVOID addr, SIZE_T length, uint prot, uint flags, fdsc_t fd, SIZE_T offset);     // Last 4 args are actually int32 (on x64 too!)   // Since kernel 2.4 glibc mmap() invokes mmap2 with an adjusted value for offset

// Deletes the mappings for the specified address range, and causes further references to addresses within the range to generate invalid memory references.
// The address addr must be a multiple of the page size (but length need not be).
static int   PXCALL munmap(PVOID addr, SIZE_T length);

static constexpr const int MREMAP_MAYMOVE = 1;
static constexpr const int MREMAP_FIXED   = 2;

static PVOID PXCALL mremap(PVOID old_address, SIZE_T old_size, SIZE_T new_size, int flags, PVOID new_address);   // LINUX specific

enum EMadv
{
 MADV_NORMAL      = 0,         // No further special treatment.
 MADV_RANDOM      = 1,         // Expect random page references.
 MADV_SEQUENTIAL  = 2,         // Expect sequential page references.
 MADV_WILLNEED    = 3,         // Will need these pages.
 MADV_DONTNEED    = 4,         // Don't need these pages.
 MADV_FREE        = DCV< 8,   5 >::V,      // Free pages only if memory pressure(or immediately?).
// Linux-cpecific
 MADV_REMOVE      = DCV< 9,   0 >::V,      // Remove these pages and resources.
 MADV_DONTFORK    = DCV< 10,  0 >::V,      // Do not inherit across fork.
 MADV_DOFORK      = DCV< 11,  0 >::V,      // Do inherit across fork.
 MADV_MERGEABLE   = DCV< 12,  0 >::V,      // KSM may merge identical pages.
 MADV_UNMERGEABLE = DCV< 13,  0 >::V,      // KSM may not merge identical pages.
 MADV_HUGEPAGE    = DCV< 14,  0 >::V,      // Worth backing with hugepages.
 MADV_NOHUGEPAGE  = DCV< 15,  0 >::V,      // Not worth backing with hugepages.
 MADV_DONTDUMP    = DCV< 16,  0 >::V,      // Explicity exclude from the core dump, overrides the coredump filter bits.
 MADV_DODUMP      = DCV< 17,  0 >::V,      // Clear the MADV_DONTDUMP flag.
 MADV_WIPEONFORK  = DCV< 18,  0 >::V,      // Zero memory on fork, child only.
 MADV_KEEPONFORK  = DCV< 19,  0 >::V,      // Undo MADV_WIPEONFORK.
 MADV_HWPOISON    = DCV< 100, 0 >::V,      // Poison a page for testing.
};

// Used to give advice or directions to the kernel about the address range beginning at address addr and with size length bytes In most cases, the goal of such advice is to improve system or application performance.
static int PXCALL madvise(PVOID addr, SIZE_T length, EMadv advice);

static int PXCALL msync(PVOID addr, SIZE_T len, int flags);
static int PXCALL mlock(PCVOID addr, SIZE_T len);
static int PXCALL munlock(PCVOID addr, SIZE_T len);

// =========================================== SOCKET ==================================
enum ESockCall  // socketcall calls  (x86_32 only)
{
 SYS_SOCKET      = 1,
 SYS_BIND        = 2,
 SYS_CONNECT     = 3,
 SYS_LISTEN      = 4,
 SYS_ACCEPT      = 5,
 SYS_GETSOCKNAME = 6,
 SYS_GETPEERNAME = 7,
 SYS_SOCKETPAIR  = 8,
 SYS_SEND        = 9,
 SYS_RECV        = 10,
 SYS_SENDTO      = 11,
 SYS_RECVFROM    = 12,
 SYS_SHUTDOWN    = 13,
 SYS_SETSOCKOPT  = 14,
 SYS_GETSOCKOPT  = 15,
 SYS_SENDMSG     = 16,
 SYS_RECVMSG     = 17,
 SYS_ACCEPT4     = 18,
 SYS_RECVMMSG    = 19,
 SYS_SENDMMSG    = 20
};

enum EShtdn
{
 SHUT_RD,
 SHUT_WR,
 SHUT_RDWR
};

enum ESockDomain
{
 AF_UNSPEC     = 0,
 AF_UNIX            = 1,    // Unix domain sockets (local pipes)
 AF_INET            = 2,    // Internet IP Protocol
// AF_AX25       =  3,  // Amateur Radio AX.25
// AF_IPX         = 4,  // Novell IPX
// AF_APPLETALK = 5,    // Appletalk DDP
//  AF_NETROM      = 6, // Amateur radio NetROM
// AF_BRIDGE       = 7, // Multiprotocol bridge
// AF_AAL5       =  8,  // Reserved for Werner's ATM
// AF_X25       =   9,  // Reserved for X.25 project
 AF_INET6       = DCV< 10,  30  >::V, // IP version 6  // Linux 10 ?
// AF_MAX         = 12, // For now..
};

enum ESockType
{
 SOCK_STREAM       = 1,  // stream socket
 SOCK_DGRAM     = 2,  // datagram socket
 SOCK_RAW         = 3,  // raw-protocol interface
 SOCK_RDM         = 4,  // reliably-delivered message
 SOCK_SEQPACKET = 5,  // sequenced packet stream
 SOCK_PACKET       = DCV< 10, 0 >::V,
};

enum ESockProto
{
 IPPROTO_IP   = 0,      // dummy for IP
 IPPROTO_ICMP = 1,      // control message protocol
 IPPROTO_IGMP = 2,      // group management protocol
 IPPROTO_GGP  = 3,      // gateway^2 (deprecated)
 IPPROTO_IPV4 = 4,      // IPv4 encapsulation
 IPPROTO_IPIP = IPPROTO_IPV4,    //for compatibility
 IPPROTO_TCP  = 6,      // tcp    // This is what you need in most cases
 IPPROTO_PUP  = 12,     // pup
 IPPROTO_UDP  = 17,     // user datagram protocol
 IPPROTO_IDP  = 22,     // xns idp
 IPPROTO_ND   = 77,     // UNOFFICIAL net disk proto

 IPPROTO_RAW  = 255,    // raw IP packet
 IPPROTO_MAX  = 256
};

enum ESockOpt
{
 SO_DEBUG      = 0x0001,          // turn on debugging info recording
 SO_ACCEPTCONN = 0x0002,          // socket has had listen()
 SO_REUSEADDR  = 0x0004,          // allow local address reuse
 SO_KEEPALIVE  = 0x0008,          // keep connections alive
 SO_DONTROUTE  = 0x0010,          // just use interface addresses
 SO_BROADCAST  = 0x0020,          // permit sending of broadcast msgs
};

using socklen_t = int;   // Should be same size as int, not size_t

// The format and size of the address is usually protocol specific.
struct sockaddr
{
 uint16 sa_family;           // address family, AF_xxx
 uint8  sa_data[14];         // 14 bytes of protocol address
};

struct msghdr
{
 PVOID      msg_name;       // Optional address
 socklen_t  msg_namelen;    // Size of address
 PIOVec     msg_iov;        // Scatter/gather array
 SIZE_T     msg_iovlen;     // # elements in msg_iov
 PVOID      msg_control;    // Ancillary data, see below
 SIZE_T     msg_controllen; // Ancillary data buffer len
 int        msg_flags;      // Flags on received message
};

using PMsgHdr    = SPTR<msghdr,   PHT>;
using PSockAddr  = SPTR<sockaddr, PHT>;
using PSockLent  = SPTR<socklen_t,PHT>;

static int PXCALL socketcall(int call, unsigned long *args);    // Deprecated?

static int PXCALL socket(ESockDomain domain, ESockType type, ESockProto protocol);
static int PXCALL connect(int sockfd, PSockAddr addr, socklen_t addrlen);
static int PXCALL bind(int sockfd, PSockAddr addr, socklen_t addrlen);
static int PXCALL accept(int sockfd, PSockAddr addr, PSockLent addrlen);
static int PXCALL accept4(int sockfd, PSockAddr addr, PSockLent addrlen, int flags=0);  // Linux x32   // INTERNAL
static int PXCALL listen(int sockfd, int backlog);
static int PXCALL shutdown(int sockfd, int how);

static int PXCALL getsockopt(int sockfd, int level, int optname, PVOID optval, PSockLent optlen);
static int PXCALL setsockopt(int sockfd, int level, int optname, PVOID optval, socklen_t optlen);

// With zero flags read and write can be used instead of recv and send.
static SSIZE_T PXCALL send(int sockfd, PVOID buf, size_t len, int flags);
static SSIZE_T PXCALL sendto(int sockfd, PVOID buf, size_t len, int flags, PSockAddr dest_addr, socklen_t addrlen);
static SSIZE_T PXCALL sendmsg(int sockfd, PMsgHdr msg, int flags);

static SSIZE_T PXCALL recv(int sockfd, PVOID buf, size_t len, int flags);
static SSIZE_T PXCALL recvfrom(int sockfd, PVOID buf, size_t len, int flags, PSockAddr src_addr, PSockLent addrlen);
static SSIZE_T PXCALL recvmsg(int sockfd, PMsgHdr msg, int flags);

// =========================================== PROCESS/THREAD/DEBUG ===========================================
// Terminates the calling process "immediately".  Any open file descriptors belonging to the process are closed.  Any children of the process are inherited by init(1) (or by the nearest "subreaper" process as defined through the use of the prctl(2) PR_SET_CHILD_SUBREAPER operation).  The process's parent is sent a SIGCHLD signal.
// The value status & 0xFF is returned to the parent process as the process's exit status, and can be collected by the parent using one of the wait(2) family of calls.
// The raw _exit() system call terminates only the calling thread, and actions such as reparenting child processes or sending SIGCHLD to the parent process are performed only if this is the last thread in the thread group.
static void PXCALL exit(int status);

// This system call is equivalent to _exit(2) except that it terminates not only the calling thread, but all threads in the calling process's thread group.
static void PXCALL exit_group(int status);

// returns the caller's thread ID (TID). In a single-threaded process, the thread ID is equal to the process ID (PID, as returned by getpid(2)).
// In a multithreaded process, all threads have the same PID, but each one has a unique TID.
static pid_t PXCALL gettid(void);      // Linux specific // MacOS have gettid name but it is related to user groups  // use pthread_self which usually a pointer or other big number
// returns the process ID (PID) of the calling process.
static pid_t PXCALL getpid(void);
// returns the process ID of the parent of the calling process.  This will be either the ID of the process that created this process using fork(),
// or, if that process has already terminated, the ID of the process to which this process has been reparented (either init(1) or a "subreaper" process defined via the prctl(2) PR_SET_CHILD_SUBREAPER operation).
// If the caller's parent is in a different PID namespace (see pid_namespaces(7)), getppid() returns 0.
static pid_t PXCALL getppid(void);

// get the process group ID of the calling process
static pid_t PXCALL getpgrp(void);    // Deprecated on ARM64

static pid_t PXCALL getpgid(pid_t pid);

// For job control
// If pgid is zero, then the PGID of the process specified by pid is made the same as its process ID
static int   PXCALL setpgid(pid_t pid, pid_t pgid);

// On success, the PID of the child process is returned in the parent, and 0 is returned in the child.  On failure, -1 is returned in the parent, no child process is created
static pid_t PXCALL vfork(void);
static pid_t PXCALL fork(void);

// The kill() system call can be used to send any signal to any process group or process.
static int   PXCALL kill(pid_t pid, int sig);

// A child created via fork(2) inherits its parent's process group ID.  The PGID is preserved across an execve(2).
static int   PXCALL execve(PCCHAR pathname, PPCHAR argv, PPCHAR envp);
// prctl is Linux only

enum ECloneFlags
{
 CLONE_SIGMSK         = 0x000000ff, // signal mask to be sent at exit
 CLONE_VM             = 0x00000100, // set if VM shared between processes
 CLONE_FS             = 0x00000200, // set if fs info shared between processes
 CLONE_FILES          = 0x00000400, // set if open files shared between processes
 CLONE_SIGHAND        = 0x00000800, // set if signal handlers and blocked signals shared
 CLONE_PIDFD          = 0x00001000, // set if a pidfd should be placed in parent
 CLONE_PTRACE         = 0x00002000, // set if we want to let tracing continue on the child too
 CLONE_VFORK          = 0x00004000, // set if the parent wants the child to wake it up on mm_release (execution of the calling process is suspended)
 CLONE_PARENT         = 0x00008000, // set if we want to have the same parent as the cloner
 CLONE_THREAD         = 0x00010000, // Same thread group?
 CLONE_NEWNS          = 0x00020000, // New mount namespace group
 CLONE_SYSVSEM        = 0x00040000, // share system V SEM_UNDO semantics
 CLONE_SETTLS         = 0x00080000, // create a new TLS for the child
 CLONE_PARENT_SETTID  = 0x00100000, // set the TID in the parent
 CLONE_CHILD_CLEARTID = 0x00200000, // clear the TID in the child
 CLONE_DETACHED       = 0x00400000, // Unused, ignored
 CLONE_UNTRACED       = 0x00800000, // set if the tracing process can't force CLONE_PTRACE on this clone
 CLONE_CHILD_SETTID   = 0x01000000, // set the TID in the child
 CLONE_NEWCGROUP      = 0x02000000, // New cgroup namespace
 CLONE_NEWUTS         = 0x04000000, // New utsname namespace
 CLONE_NEWIPC         = 0x08000000, // New ipc namespace
 CLONE_NEWUSER        = 0x10000000, // New user namespace
 CLONE_NEWPID         = 0x20000000, // New pid namespace
 CLONE_NEWNET         = 0x40000000, // New network namespace
 CLONE_IO             = 0x80000000, // Clone io context
};
// https://github.com/raspberrypi/linux/blob/rpi-5.15.y/kernel/fork.c
// glibc/glibc/sysdeps/unix/sysv/linux/x86_64/clone.S.html
// It returns 0 in the child process and returns the PID of the child in the parent.
static pid_t  PXCALL cloneB0(uint32 flags, PVOID newsp, PINT32 parent_tid, PINT32 child_tid, PVOID tls);  // Linux specific  // x86-x64, ...            // struct user_desc* tls     // Use this as a generic definition
static pid_t  PXCALL cloneB1(uint32 flags, PVOID newsp, PINT32 parent_tid, PVOID tls, PINT32 child_tid);  // Linux specific  // x86-32, ARM32, ARM64, ...

// Spawn a new process
// Default format of siofd is {oldfd,newfd,...,-1}
// NOTE: Values other than -1 can be used in the future to do some actions between vfork and execve in format {ACTIONID1,OPTVAL1,...OPTVALN,ACTIONID2,OPTVAL,ACTIONID3,-1}
// Current working directory is inherited (Same as fork/vfork)
// siofd - list of file descriptors to share (Last is -1) to make some descriptor of current process into expected descriptor of a new process (IO redirection)
//  On Windows maps only STDIN, STDOUT, STDERR to PEB anything else just duplicates
// See posix_spawn for actions that may be required
static pid_t  PXCALL spawn(PCCHAR path, PPCHAR argv, PPCHAR envp, PINT32 siofd, uint32 flags);    // Improvised  // int siofd[3]{STDIN,STDOUT,STDERR} // Flags are additional flags for Clone, unused for now

static pid_t  PXCALL thread(NTHD::PThreadProc Proc, PVOID Data, SIZE_T DatSize, SIZE_T StkSize, SIZE_T TlsSize, uint32 Flags);  // Improvised   // Actually allocates: PageAlign(Size+StkSize+TlsSize+ThreadRec)
static sint   PXCALL thread_sleep(uint64 ns);     // nleepns???	  // -1 - wait infinitely  // Sleeps on its futex	 // -1 sleep until termination
static sint   PXCALL thread_exit(sint status);    // Allows to exit without returning from ThreadProc directly	   // Deallocate the thread`s stack memory?
static sint   PXCALL thread_status(pid_t thid);	  // Use after waiting for a thread to finish
static sint   PXCALL thread_kill(pid_t thid, sint status);    // Terminates the thread
static sint   PXCALL thread_wait(pid_t thid, uint64 ns);      // Wati for the thread termination  // -1 - wait infinitely
static sint   PXCALL thread_alert(pid_t thid, uint32 code);   // Wakes the thread from any wait	(thread_sleep)	// Like pthread_cancel	 // Cancel IO?
static sint   PXCALL thread_affinity_set(pid_t thid, uint64 mask, uint32 from); 
static sint   PXCALL thread_affinity_get(pid_t thid, size_t buf_len, PSIZE_T buf); 
// CPU affinity
// Suspend/Resume
// Terminate
// Exit?   (Not exit_group which terminates the entire app)

static constexpr const int  WNOHANG    = 1;   // Don't block waiting.
static constexpr const int  WUNTRACED  = 2;   // Report status of stopped children.
static constexpr const int  WCONTINUED = 3;   // Return if a stopped child has been resumed by delivery of SIGCONT

static constexpr _finline int32 WEXITSTATUS(int32 s) {return (((s) & 0xff00) >> 8);}
static constexpr _finline int32 WTERMSIG(int32 s) {return ((s) & 0x7f);}
static constexpr _finline int32 WSTOPSIG(int32 s) {return WEXITSTATUS(s);}
static constexpr _finline bool  WCOREDUMP(int32 s) {return ((s) & 0x80);}
static constexpr _finline bool  WIFEXITED(int32 s) {return (!WTERMSIG(s));}		// returns true if the child terminated normally, that is, by calling exit(3) or _exit(2), or by returning from main().
static constexpr _finline bool  WIFSTOPPED(int32 s) {return  ((short)((((s)&0xffff)*0x10001U)>>8) > 0x7f00);}
static constexpr _finline bool  WIFSIGNALED(int32 s) {return (((s)&0xffff)-1U < 0xffu);}
static constexpr _finline bool  WIFCONTINUED(int32 s) {return ((s) == 0xffff);}

/*
When a process terminates its parent process must acknowledge this using the wait or waitpid function. These functions also return the exit status.

A child that terminates, but has not been waited for becomes a "zombie". The kernel maintains a minimal set of information about the
zombie process (PID, termination status, resource usage information) in order to allow the parent to later perform a wait to obtain
information about the child. As long as a zombie is not removed from the system via a wait, it will consume a slot in the kernel
process table, and if this table fills, it will not be possible to create further processes. If a parent process terminates,
then its "zombie" children (if any) are adopted by init(8), which automatically performs a wait to remove the zombies.

The wait() system call suspends execution of the calling process until one of its children terminates. The call wait(&status) is equivalent to: waitpid(-1, &status, 0);
The waitpid() system call suspends execution of the calling process until a child specified by pid argument has changed state.

https://stackoverflow.com/questions/18441760/linux-where-are-the-return-codes-stored-of-system-daemons-and-other-processes?noredirect=1&lq=1
/proc/[pid]/stat
kill(getpid(), SIGKILL);
*/
static pid_t  PXCALL wait4(pid_t pid, PINT32 wstatus, int options, PVOID rusage);    // Old, should use  waitpid or waitid
//static int    PXCALL waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);   // Later


enum EFutex
{
 FUTEX_WAIT            = 0,    // BSD
 FUTEX_WAKE            = 1,    // BSD
 FUTEX_FD              = 2,    // Probably can be done on Windows
 FUTEX_REQUEUE         = 3,    // BSD (Can it be done on Windows?)
 FUTEX_CMP_REQUEUE     = 4,
 FUTEX_WAKE_OP         = 5,
 FUTEX_LOCK_PI         = 6,
 FUTEX_UNLOCK_PI       = 7,
 FUTEX_TRYLOCK_PI      = 8,
 FUTEX_WAIT_BITSET     = 9,
 FUTEX_WAKE_BITSET     = 10,
 FUTEX_WAIT_REQUEUE_PI = 11,
 FUTEX_CMP_REQUEUE_PI  = 12,
 FUTEX_LOCK_PI2        = 13,
 FUTEX_PRIVATE_FLAG    = 128,	  // Linux, new
 FUTEX_CLOCK_REALTIME  = 256,
 FUTEX_CMD_MASK        = ~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME),

 FUTEX_WAIT_PRIVATE = (FUTEX_WAIT | FUTEX_PRIVATE_FLAG),
 FUTEX_WAKE_PRIVATE = (FUTEX_WAKE | FUTEX_PRIVATE_FLAG),
 FUTEX_REQUEUE_PRIVATE = (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG),

 FUTEX_OP_SET       = 0,   // *(int *)UADDR2  = OPARG;
 FUTEX_OP_ADD       = 1,   // *(int *)UADDR2 += OPARG; 
 FUTEX_OP_OR        = 2,   // *(int *)UADDR2 |= OPARG; 
 FUTEX_OP_ANDN      = 3,   // *(int *)UADDR2 &= ~OPARG; 
 FUTEX_OP_XOR       = 4,   // *(int *)UADDR2 ^= OPARG; 

 FUTEX_OP_OPARG_SHIFT = 8,   // Use (1 << OPARG) instead of OPARG. 

 FUTEX_OP_CMP_EQ    = 0,   // if (oldval == CMPARG) wake 
 FUTEX_OP_CMP_NE    = 1,   // if (oldval != CMPARG) wake 
 FUTEX_OP_CMP_LT    = 2,   // if (oldval <  CMPARG) wake 
 FUTEX_OP_CMP_LE    = 3,   // if (oldval <= CMPARG) wake 
 FUTEX_OP_CMP_GT    = 4,   // if (oldval >  CMPARG) wake 
 FUTEX_OP_CMP_GE    = 5,   // if (oldval >= CMPARG) wake 
};

// Before the thread is suspended the value of the futex variable is checked. If it does not have the same value as the val1 parameter the system call immediately returns with the error EWOULDBLOCK.
// If the time runs out without a notification being sent, the system call returns with the error ETIMEDOUT
// system call can return if the thread received a signal. In this case the error is EINTR.
// for FUTEX_WAIT, timeout is interpreted as a relative value.  This differs from other futex operations, where timeout is interpreted as an absolute value.
// https://man7.org/linux/man-pages/man2/futex.2.html
// 
// Note that a wake-up can also be caused by common futex usage patterns in unrelated code that happened to have previously used the
//   futex word's memory location (e.g., typical futex-based implementations of Pthreads mutexes can cause this under some conditions).  Therefore, callers should always
//   conservatively assume that a return value of 0 can mean a spurious wake-up, and use the futex word's value (i.e., the user-space synchronization scheme) to decide whether to continue to block or not.
//
static sint32 PXCALL futexGD(PUINT32 uaddr, int op, uint32 val, const PTiSp timeout);	  // Minimal operation
static sint32 PXCALL futex(PUINT32 uaddr, int op, uint32 val, const PTiSp timeout, PUINT32 uaddr2, uint32 val3);    // Linux   // timespec may be uint32_t val2 (see op)     // Returns long
// OpenBSD:      int futex(PUINT32 uaddr, int op, uint32 val, const PTiSp timeout, PUINT32 uaddr2);
// BSD:		  https://github.com/mumble-voip/sbcelt/blob/master/lib/futex-freebsd.c
/*int futex_wake(int *futex) {return _umtx_op(futex, UMTX_OP_WAKE, 1, 0, 0);}

int futex_wait(int *futex, int val, struct timespec *ts) {
	int err = _umtx_op(futex, UMTX_OP_WAIT_UINT, val, 0, (void *)ts);
	if (err != 0) {
		if (errno == ETIMEDOUT)return FUTEX_TIMEDOUT;	
		 else if (errno == EINTR)return FUTEX_INTERRUPTED; // XXX: unsure if umtx can be EINTR'd.	
	}
	return err;
} */


};

using PX   = NPOSIX<uint>;   // Current build
using PX64 = NPOSIX<uint64>;
//============================================================================================================
