#ifdef _WIN32

#define _UNICODE
#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <c42.h>
#include "c42svc.h"

#if !C42_BSLE
#error AFAIK Windows only works on little endian boxes
#endif

/* file_t *******************************************************************/
typedef struct file_s file_t;
struct file_s
{
    c42_io8_t io8;
    HANDLE h;
};

/* file_read ****************************************************************/
uint_fast8_t C42_CALL file_read
(
    c42_io8_t * io, 
    uint8_t * data, 
    size_t size, 
    size_t * rsize
)
{
    file_t * f = (file_t *) io;
    DWORD r;
    BOOL b;

    b = ReadFile(f->h, data, size, &r, NULL);
    *rsize = r;
    if (b) return 0;
    /* TODO: provide specific error codes */
    return C42_IO8_OTHER_ERROR;
}

/* file_write ***************************************************************/
uint_fast8_t C42_CALL file_write
(
    c42_io8_t * io,
    uint8_t const * data,
    size_t size,
    size_t * wsize
)
{
    file_t * f = (file_t *) io;
    BOOL b;
    DWORD s;

    s = (DWORD) size;
#if SIZE_MAX > UINT32_MAX
    if ((size_t) s != size) return C42_IO8_TOO_BIG;
#endif
    b = WriteFile(f->h, data, s, &s, NULL);
    *wsize = s;
    if (b) return 0;
    /* TODO: provide specific error codes */
    return C42_IO8_OTHER_ERROR;
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
    LONG hi;
    DWORD e, dw;

    hi = (offset >> 31) >> 1;
    dw = SetFilePointer(f->h, (LONG) offset, &hi, anchor);
    if (dw == INVALID_SET_FILE_POINTER && (e = GetLastError() != NO_ERROR))
    {
        switch (e)
        {
        case ERROR_NEGATIVE_SEEK: return C42_IO8_BAD_POS;
        default: return C42_IO8_OTHER_ERROR;
        }
    }
#if PTRDIFF_MAX == INT32_MAX
    if (hi) return C42_IO8_POS_OVERFLOW;
#endif
    *pos = dw;
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
    LONG hi;
    DWORD e, dw;

    hi = (offset >> 31) >> 1;
    dw = SetFilePointer(f->h, (LONG) offset, &hi, anchor);
    if (dw == INVALID_SET_FILE_POINTER && (e = GetLastError() != NO_ERROR))
    {
        switch (e)
        {
        case ERROR_NEGATIVE_SEEK: return C42_IO8_BAD_POS;
        default: return C42_IO8_OTHER_ERROR;
        }
    }
    *pos = dw | ((uint64_t) hi << 32);
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
void file_init (file_t * f, HANDLE h)
{
    f->io8.io8_class = &file_class;
    f->h = h;
}

/* ma_handler ***************************************************************/
uint_fast8_t C42_CALL ma_handler
(
    void * * ptr_p,
    size_t old_size,
    size_t new_size,
    void * ctx
)
{
    HANDLE h = (HANDLE) ctx;
    if (!old_size)
    {
        // alloc
        if (!new_size) { *ptr_p = NULL; return 0; }
        if (!(*ptr_p = HeapAlloc(h, 0, new_size))) return C42_MA_NO_MEM;
    }
    else if (new_size)
    {
        // realloc
        if (!(*ptr_p = HeapReAlloc(h, 0, *ptr_p, new_size))) 
            return C42_MA_NO_MEM;
    }
    else
    {
        // free
        HeapFree(h, 0, *ptr_p);
        *ptr_p = NULL;
    }
    return 0;
}

/* c42svc_ma ****************************************************************/
C42SVC_API uint_fast8_t C42_CALL c42svc_ma (c42_ma_t * ma, char const * name)
{
    if (!name || !strcmp(name, "win32heap"))
    {
        ma->handler = ma_handler;
        ma->ctx = GetProcessHeap();
        return 0;
    }

    return C42SVC_MISSING;
}


/* c42svc_fsi ***************************************************************/
C42SVC_API uint_fast8_t C42_CALL c42svc_fsi (c42_fsi_t * fsi, char const * name)
{
    (void) fsi;
    if (!name || !strcmp(name, "posix"))
    {
        return C42SVC_MISSING;
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

