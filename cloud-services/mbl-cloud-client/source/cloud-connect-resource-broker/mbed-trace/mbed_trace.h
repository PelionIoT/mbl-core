
#ifndef _A_
#define _A_

#include <string.h>
#define tr_debug(...)
#define tr_warn(...)
#define tr_warning(...)
#define tr_info(...)
#define tr_error(...)
#define tr_err(...)
#define strerror(x) (void*)x
#define MblError_to_str(x) (void*)x
#define TRACE_LEVEL_DEBUG 1
#define TRACE_LEVEL_INFO 2
#define TRACE_LEVEL_ERROR 3
#define TRACE_LEVEL_WARN 4
#endif