#include <errno.h>

#include "interceptors.h"
#include "interceptors.def" /* To get the headers for intercepted functions */
#undef INTERCEPT /* To suppress the intercept macros... */
#include "intercept.h" /* ...while importing declarations and enum */

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

void intercept_i_after(enum intercept_funcs func, int after, int fail_errno, int retval) {
  if (func >= 0 && func < INTERCEPTED_COUNT)
    failures[func] = (struct interception){
      .fail_after = after,
      .fail_errno = fail_errno,
      .fail_retval.i = retval
    };
}

void intercept_ptr_after(enum intercept_funcs func, int after, int fail_errno, void *retval) {
  if (func >= 0 && func < INTERCEPTED_COUNT)
    failures[func] = (struct interception){
      .fail_after = after,
      .fail_errno = fail_errno,
      .fail_retval.ptr = retval
    };
}

void intercept_sst_after(enum intercept_funcs func, int after, int fail_errno, ssize_t retval) {
  if (func >= 0 && func < INTERCEPTED_COUNT)
    failures[func] = (struct interception){
      .fail_after = after,
      .fail_errno = fail_errno,
      .fail_retval.sst = retval
    };
}

#define INTERCEPTION(func_name) failures[INTERCEPT_IDX(func_name)]
#define EXPAND_INTERCEPTOR(ret_type, func_name, derr, dret, ...)        \
  ret_type INTERCEPT_FUNC(func_name)(GET_TYPES(__VA_ARGS__)) {          \
    if (INTERCEPTION(func_name).fail_after--)                           \
      return func_name(GET_VARS(__VA_ARGS__));                          \
                                                                        \
    INTERCEPTION(func_name).fail_after = 0;                             \
    errno = INTERCEPTION(func_name).fail_errno;                         \
    return (*(ret_type *)&(INTERCEPTION(func_name).fail_retval));       \
  }
#include "interceptors.def"
#undef INTERCEPTION
