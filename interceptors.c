#include "interceptors.h"
#include <errno.h>

#include "interceptors.def" /* To get the headers for intercepted functions */

struct interception {
  int fail_after;
  int fail_errno;
  union {
    int i;
    void *ptr;
    ssize_t sst;
  } fail_retval;
};

static struct interception failures[INTERCEPTED_COUNT];

void intercept_i_after(enum intercepted_function func, int after, int fail_errno, int retval) {
  if (func >= 0 && func < INTERCEPTED_COUNT)
    failures[func] = (struct interception){
      .fail_after = after,
      .fail_errno = fail_errno,
      .fail_retval.i = retval
    };
}

void intercept_ptr_after(enum intercepted_function func, int after, int fail_errno, void *retval) {
  if (func >= 0 && func < INTERCEPTED_COUNT)
    failures[func] = (struct interception){
      .fail_after = after,
      .fail_errno = fail_errno,
      .fail_retval.ptr = retval
    };
}

void intercept_sst_after(enum intercepted_function func, int after, int fail_errno, ssize_t retval) {
  if (func >= 0 && func < INTERCEPTED_COUNT)
    failures[func] = (struct interception){
      .fail_after = after,
      .fail_errno = fail_errno,
      .fail_retval.sst = retval
    };
}

#define EXPAND_INTERCEPTOR(return_type, function_name, ...)             \
  return_type CONCAT(INTERCEPTOR_PREFIX, function_name)(GET_TYPES(__VA_ARGS__)) { \
    if (failures[INTERCEPTED_ ## function_name].fail_after--)           \
      return function_name(GET_VARS(__VA_ARGS__));                      \
                                                                        \
    failures[INTERCEPTED_ ## function_name].fail_after = 0;             \
    errno = failures[INTERCEPTED_ ## function_name].fail_errno;         \
    return (*(return_type *)&(failures[INTERCEPTED_ ## function_name].fail_retval)); \
  }
#include "interceptors.def"
