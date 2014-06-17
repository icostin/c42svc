#ifndef C42SVC_H
#define C42SVC_H

#include <c42.h>

#if C42_STATIC
#define C42SVC_API
#elif C42_LIB_BUILD
#define C42SVC_API C42_LIB_EXPORT
#else
#define C42SVC_API C42_LIB_IMPORT
#endif

#define C42SVC_OK 0 /**< ok */
#define C42SVC_INIT_FAILED 1 /**< init failed */
#define C42SVC_MISSING 2 /**< requested item is missing */
#define C42SVC_UNSUP 3 /**< unsupported feature */

/* c42svc_ma ****************************************************************/
/**
 *  Inits a memory allocator.
 *  @param ma   [out]   allocator struct
 *  @param name [in]    the allocator name
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_ma (c42_ma_t * ma, char const * name);

/* c42svc_fsi ***************************************************************/
/**
 *  Inits file system interface.
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_fsi (c42_fsi_t * fsi, char const * name);

/* c42svc_smt ***************************************************************/
/**
 *  Inits simple multithreading interface.
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_smt (c42_smt_t * smt, char const * name);


#endif

