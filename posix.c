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

/* file_t *******************************************************************/
typedef struct file_s file_t;
struct file_s
{
    c42_io8_t io8;
    int fd;
};

/* file_read ****************************************************************/
uint_fast8_t file_read
    (c42_io8_t * io, uint8_t * data, size_t size, size_t * rsize)
{
    file_t * f = (file_t *) io;
    ssize_t s;

    s = read(f->fd, data, size);
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
uint_fast8_t file_write
    (c42_io8_t * io, uint8_t const * data, size_t size, size_t * wsize)
{
    file_t * f = (file_t *) io;
    ssize_t s;

    s = write(f->fd, data, size);
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
uint_fast8_t C42_CALL file_seek
(
    c42_io8_t * io,
    ptrdiff_t offset,
    int anchor,
    size_t * pos
)
{
    file_t * f = (file_t *) io;
    off_t off;
    off = lseek(f->fd, (off_t) offset, anchor);
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
uint_fast8_t C42_CALL file_seek64
(
    c42_io8_t * io,
    int64_t offset,
    int anchor,
    uint64_t * pos
)
{
    file_t * f = (file_t *) io;
    off64_t off;
    off = lseek64(f->fd, (off64_t) offset, anchor);
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

/* file_class ***************************************************************/
c42_io8_class_t file_class =
{
    file_read,
    file_write,
    file_seek,
    file_seek64,
    NULL,
    NULL
};

/* file_init ****************************************************************/
void file_init (file_t * f, int fd)
{
    f->io8.io8_class = &file_class;
    f->fd = fd;
}

/* file_alloc ***************************************************************/
c42_io8_t * file_alloc (int fd)
{
    file_t * f;
    f = malloc(sizeof(file_t));
    if (!f) return NULL;
    file_init(f, fd);
    return &f->io8;
}

/* file_open ****************************************************************/
uint_fast8_t C42_CALL file_open
(
    c42_io8_t * * io_p,
    uint8_t const * path,
    int mode,
    void * context
)
{
    int oflags, omode, fd;
    (void) context;
    switch (mode & 7)
    {
    case C42_FSI_OPEN_EXISTING:
        oflags = 0;
        break;
    case C42_FSI_OPEN_ALWAYS:
        oflags = O_CREAT;
        break;
    case C42_FSI_CREATE_NEW:
        oflags = O_CREAT | O_EXCL;
        break;
    case C42_FSI_CREATE_ALWAYS:
        oflags = O_CREAT | O_TRUNC;
        break;
    case C42_FSI_TRUNC_EXISTING:
        oflags = O_TRUNC;
        break;
    default:
        return C42_FSI_BAD_MODE;
    }
    switch (mode & (C42_FSI_READ | C42_FSI_WRITE))
    {
    case 0:
    case C42_FSI_READ: 
        oflags |= O_RDONLY;
        break;
    case C42_FSI_WRITE:
        oflags |= O_WRONLY;
        break;
    case C42_FSI_READ | C42_FSI_WRITE:
        oflags |= O_RDWR;
        break;
    }
    omode = mode >> C42_FSI_PERM_SHIFT;

    fd = open((char const *) path, oflags, omode);
    if (fd < 0) return C42_FSI_SOME_ERROR;

    *io_p = file_alloc(fd);
    if (!*io_p) 
    {
        while (close(fd) < 0 && errno == EINTR); 
        return C42_FSI_NO_MEM; 
    }
    return 0;
}

/* fsi **********************************************************************/
static c42_fsi_t posix_fsi =
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

/* c42svc_ma ****************************************************************/
C42SVC_API uint_fast8_t C42_CALL c42svc_ma (c42_ma_t * ma, char const * name)
{
    if (!name || !strcmp(name, "libc"))
    {
        *ma = libc_ma;
        return 0;
    }

    return C42SVC_MISSING;
}


/* c42svc_fsi ***************************************************************/
C42SVC_API uint_fast8_t C42_CALL c42svc_fsi (c42_fsi_t * fsi, char const * name)
{
    if (!name || !strcmp(name, "posix"))
    {
        *fsi = posix_fsi;
        return 0;
    }

    return C42SVC_MISSING;
}


/* c42svc_smt ***************************************************************/
/**
 *  Inits simple multithreading interface.
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_smt (c42_smt_t * smt, char const * name)
{
    (void) smt;
    (void) name;
    return C42SVC_MISSING;
}

#endif
