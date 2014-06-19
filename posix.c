#ifndef _WIN32
#define _LARGEFILE64_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "c42svc.h"

/* file_read ****************************************************************/
static uint_fast8_t file_read
(
    uintptr_t context,
    uint8_t * data,
    size_t size,
    size_t * rsize
)
{
    int fd = context;
    ssize_t s;

    s = read(fd, data, size);
    if (s < 0)
    {
        switch (errno)
        {
        case EAGAIN:
#if EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
            return C42_IO8_WOULD_BLOCK;
        case EBADF: return C42_IO8_BAD_FILE;
        case EFAULT: return C42_IO8_BAD_ADDR;
        case EINTR: return C42_IO8_INTERRUPTED;
        case EINVAL: return C42_IO8_BAD_WRITE;
        case EIO: return C42_IO8_IO_ERROR;
        case EISDIR: return C42_IO8_IS_DIR;
        default: return C42_IO8_OTHER_ERROR;
        }
    }
    *rsize = (size_t) s;
    return 0;
}

/* file_write ***************************************************************/
static uint_fast8_t file_write
(
    uintptr_t context,
    uint8_t const * data,
    size_t size,
    size_t * wsize
)
{
    int fd = context;
    ssize_t s;

    s = write(fd, data, size);
    if (s < 0)
    {
        switch (errno)
        {
        case EAGAIN:
#if EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
            return C42_IO8_WOULD_BLOCK;
        case EBADF: return C42_IO8_BAD_FILE;
        case EFAULT: return C42_IO8_BAD_ADDR;
        case EFBIG: return C42_IO8_TOO_BIG;
        case EINTR: return C42_IO8_INTERRUPTED;
        case EINVAL: return C42_IO8_BAD_WRITE;
        case EIO: return C42_IO8_IO_ERROR;
        case ENOSPC: return C42_IO8_NO_SPACE;
        case EPIPE: return C42_IO8_BROKEN_PIPE;
        default: return C42_IO8_OTHER_ERROR;
        }
    }
    *wsize = (size_t) s;
    return 0;
}

/* file_seek ****************************************************************/
static uint_fast8_t C42_CALL file_seek
(
    uintptr_t context,
    ptrdiff_t offset,
    int anchor,
    size_t * pos
)
{
    int fd = context;
    off_t off;
    off = lseek(fd, (off_t) offset, anchor);
    if (off == (off_t) -1)
    {
        switch (errno)
        {
        case EBADF: return C42_IO8_BAD_FILE;
        case EINVAL: return C42_IO8_BAD_POS;
        case EOVERFLOW: return C42_IO8_POS_OVERFLOW;
        case EPIPE: return C42_IO8_NO_SEEK;
        default: return C42_IO8_OTHER_ERROR;
        }
    }
    *pos = off;
    return 0;
}

/* file_seek64 **************************************************************/
static uint_fast8_t C42_CALL file_seek64
(
    uintptr_t context,
    int64_t offset,
    int anchor,
    uint64_t * pos
)
{
    int fd = context;
    off64_t off;
    off = lseek64(fd, (off64_t) offset, anchor);
    if (off == (off64_t) -1)
    {
        switch (errno)
        {
        case EBADF: return C42_IO8_BAD_FILE;
        case EINVAL: return C42_IO8_BAD_POS;
        case EOVERFLOW: return C42_IO8_POS_OVERFLOW;
        case EPIPE: return C42_IO8_NO_SEEK;
        default: return C42_IO8_OTHER_ERROR;
        }
    }
    *pos = off;
    return 0;
}

/* file_close ***************************************************************/
static uint_fast8_t C42_CALL file_close
(
    uintptr_t context,
    int mode
)
{
    int fd = context;
    if (mode != (C42_IO8_OP_READ | C42_IO8_OP_WRITE)) return C42_IO8_NA;
    if (close(fd))
    {
        switch (errno)
        {
        case EBADF: return C42_IO8_BAD_FILE;
        case EINTR: return C42_IO8_INTERRUPTED;
        case EIO: return C42_IO8_IO_ERROR;
        default: return C42_IO8_OTHER_ERROR;
        }
    }
    return 0;
}

/* file_class ***************************************************************/
c42_io8_class_t file_class =
{
    file_read,
    file_write,
    file_seek,
    file_seek64,
    NULL,
    file_close
};

/* file_init ****************************************************************/
void file_init (c42_io8_t * f, int fd)
{
    f->io8_class = &file_class;
    f->context = fd;
}

/* file_open ****************************************************************/
uint_fast8_t C42_CALL file_open
(
    c42_io8_t * io,
    uint8_t const * path,
    int mode,
    void * context
)
{
    int oflags, omode, fd;
    (void) context;
    switch (mode & 7)
    {
    case C42_FSA_OPEN_EXISTING:
        oflags = 0;
        break;
    case C42_FSA_OPEN_ALWAYS:
        oflags = O_CREAT;
        break;
    case C42_FSA_CREATE_NEW:
        oflags = O_CREAT | O_EXCL;
        break;
    case C42_FSA_CREATE_ALWAYS:
        oflags = O_CREAT | O_TRUNC;
        break;
    case C42_FSA_TRUNC_EXISTING:
        oflags = O_TRUNC;
        break;
    default:
        return C42_FSA_BAD_MODE;
    }
    switch (mode & (C42_FSA_READ | C42_FSA_WRITE))
    {
    case 0:
    case C42_FSA_READ: 
        oflags |= O_RDONLY;
        break;
    case C42_FSA_WRITE:
        oflags |= O_WRONLY;
        break;
    case C42_FSA_READ | C42_FSA_WRITE:
        oflags |= O_RDWR;
        break;
    }
    omode = mode >> C42_FSA_PERM_SHIFT;

    fd = open((char const *) path, oflags, omode);
    if (fd < 0) return C42_FSA_SOME_ERROR;

    file_init(io, fd);
    return 0;
}

/* fsi **********************************************************************/
static c42_fsa_t posix_fsa =
{
    file_open,
    NULL, // file_open context
};

/* ma_handler ***************************************************************/
uint_fast8_t C42_CALL ma_handler
(
    void * * ptr_p,
    size_t old_size,
    size_t new_size,
    void * ctx
)
{
    (void) ctx;
    if (!old_size)
    {
        // alloc
        if (!new_size) { *ptr_p = NULL; return 0; }
        if (!(*ptr_p = malloc(new_size))) return C42_MA_NO_MEM;
    }
    else if (new_size)
    {
        // realloc
        if (!(*ptr_p = realloc(*ptr_p, new_size))) return C42_MA_NO_MEM;
    }
    else
    {
        // free
        free(*ptr_p);
        *ptr_p = NULL;
    }
    return 0;
}

/* ma ***********************************************************************/
static c42_ma_t libc_ma =
{
    ma_handler,
    NULL
};

/* c42svc_init **************************************************************/
/**
 *  Services
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_init
(
    c42_svc_t * svc
)
{
    svc->provider = (uint8_t const *) "c42svc-posix-v0000-"
#if C42_ARM32
        "-arm32"
#elif C42_ARM64
        "-arm64"
#elif C42_MIPS
        "-mips"
#elif C42_AMD64
        "-amd64"
#elif C42_IA32
        "-ia32"
#else
        "-unknown_arch"
#endif

#if C42_BSLE
        "-bsle"
#elif C42_BSBE
        "-bsbe"
#elif C42_WSLE
        "-wsle"
#elif C42_WSBE
        "-wsbe"
#endif

#if C42_STATIC
        "-static"
#else
        "-dynamic"
#endif

#if _DEBUG
        "-debug"
#else
        "-release"
#endif
        ;
    svc->ma = &libc_ma;
    svc->smt = NULL;
    svc->fsa = &posix_fsa;
    return 0;
}


/* c42svc_std_init **********************************************************/
/**
 *  Inits standard streams.
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_std_init
(
    c42_io8_std_t * stdio
)
{
    (void) stdio;
    return C42SVC_UNSUP;
}

C42SVC_API uint_fast8_t C42_CALL c42svc_std_finish
(
    c42_io8_std_t * stdio
)
{
    (void) stdio;
    return C42SVC_UNSUP;
}

#endif
