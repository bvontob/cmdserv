#ifndef INTERCEPT_H
#define INTERCEPT_H

#include "interceptors.h"

enum intercept_funcs {
  INTERCEPTED_INVALID = -1,
#define EXPAND_INTERCEPTOR(ret_type, func_name, derr, dret, ...)        \
  INTERCEPT_IDX(func_name),
#include "interceptors.def"
  INTERCEPTED_COUNT
};

#ifdef INTERCEPT
#define calloc(...)        INTERCEPT_FUNC(calloc)(__VA_ARGS__)
#define malloc(...)        INTERCEPT_FUNC(malloc)(__VA_ARGS__)
#define realloc(...)       INTERCEPT_FUNC(realloc)(__VA_ARGS__)
#define select(...)        INTERCEPT_FUNC(select)(__VA_ARGS__)
#define socket(...)        INTERCEPT_FUNC(socket)(__VA_ARGS__)
#define recv(...)          INTERCEPT_FUNC(recv)(__VA_ARGS__)
#define setsockopt(...)    INTERCEPT_FUNC(setsockopt)(__VA_ARGS__)
#define listen(...)        INTERCEPT_FUNC(listen)(__VA_ARGS__)
#define bind(...)          INTERCEPT_FUNC(bind)(__VA_ARGS__)
#define accept(...)        INTERCEPT_FUNC(accept)(__VA_ARGS__)
#define getnameinfo(...)   INTERCEPT_FUNC(getnameinfo)(__VA_ARGS__)
#endif /* INTERCEPT */

#define EXPAND_INTERCEPTOR(ret_type, func_name, derr, dret, ...)        \
  ret_type INTERCEPT_FUNC(func_name)(GET_TYPES(__VA_ARGS__));
#include "interceptors.def"

#endif /* INTERCEPT_H */
