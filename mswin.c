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

/* file_read ****************************************************************/
uint_fast8_t C42_CALL file_read
(
    uintptr_t context,
    uint8_t * data, 
    size_t size, 
    size_t * rsize
)
{
    HANDLE h = (HANDLE) context;
    DWORD r;
    BOOL b;

    b = ReadFile(h, data, size, &r, NULL);
    *rsize = r;
    if (b) return 0;
    /* TODO: provide specific error codes */
    return C42_IO8_OTHER_ERROR;
}

/* file_write ***************************************************************/
uint_fast8_t C42_CALL file_write
(
    uintptr_t context,
    uint8_t const * data,
    size_t size,
    size_t * wsize
)
{
    HANDLE h = (HANDLE) context;
    BOOL b;
    DWORD s;

    s = (DWORD) size;
#if SIZE_MAX > UINT32_MAX
    if ((size_t) s != size) return C42_IO8_TOO_BIG;
#endif
    b = WriteFile(h, data, s, &s, NULL);
    *wsize = s;
    if (b) return 0;
    /* TODO: provide specific error codes */
    return C42_IO8_OTHER_ERROR;
}

/* file_seek ****************************************************************/
uint_fast8_t C42_CALL file_seek
(
    uintptr_t context,
    ptrdiff_t offset,
    int anchor,
    size_t * pos
)
{
    HANDLE h = (HANDLE) context;
    LONG hi;
    DWORD e, dw;

    hi = (offset >> 31) >> 1;
    dw = SetFilePointer(h, (LONG) offset, &hi, anchor);
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
    uintptr_t context,
    int64_t offset,
    int anchor,
    uint64_t * pos
)
{
    HANDLE h = (HANDLE) context;
    LONG hi;
    DWORD e, dw;

    hi = (offset >> 31) >> 1;
    dw = SetFilePointer(h, (LONG) offset, &hi, anchor);
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
void file_init (c42_io8_t * f, HANDLE h)
{
    f->io8_class = &file_class;
    f->context = (uintptr_t) h;
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

/* c42svc_init **************************************************************/
C42SVC_API uint_fast8_t C42_CALL c42svc_init
(
    c42_svc_t * svc
)
{
    svc->provider = (uint8_t const *) "c42svc-mswin-v0000-"
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
    svc->ma.handler = ma_handler;
    svc->ma.context = (void *) GetProcessHeap();
    C42_VAR_CLEAR(svc->smt);
    C42_VAR_CLEAR(svc->fsa);
    return 0;
}


/* c42svc_std_init **********************************************************/
C42SVC_API uint_fast8_t C42_CALL c42svc_std_init
(
    c42_io8_std_t * stdio
)
{
    file_init(&stdio->in, GetStdHandle(STD_INPUT_HANDLE));
    file_init(&stdio->out, GetStdHandle(STD_OUTPUT_HANDLE));
    file_init(&stdio->err, GetStdHandle(STD_ERROR_HANDLE));
    return 0;
}

/* c42svc_std_finish ********************************************************/
C42SVC_API uint_fast8_t C42_CALL c42svc_std_finish
(
    c42_io8_std_t * stdio
)
{
    (void) stdio;
    return 0;
}

#endif

