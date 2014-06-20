#ifndef C42SVC_H
#define C42SVC_H

#include <c42.h>

#if C42SVC_STATIC
#define C42SVC_API
#elif C42SVC_LIB_BUILD
#define C42SVC_API C42_LIB_EXPORT
#else
#define C42SVC_API C42_LIB_IMPORT
#endif

#define C42SVC_OK 0 /**< ok */
#define C42SVC_INIT_FAILED 1 /**< init failed */
#define C42SVC_MISSING 2 /**< requested item is missing */
#define C42SVC_UNSUP 3 /**< unsupported feature */

/* c42svc_init **************************************************************/
/**
 *  Services
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_init
(
    c42_svc_t * svc
);

/* c42svc_std_init **********************************************************/
/**
 *  Inits standard streams.
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_std_init
(
    c42_io8_std_t * stdio
);

/* c42svc_std_finish ********************************************************/
/**
 *  Finishes standard streams.
 */
C42SVC_API uint_fast8_t C42_CALL c42svc_std_finish
(
    c42_io8_std_t * stdio
);

#endif

