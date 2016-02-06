#ifndef INTERCEPT_H
#define INTERCEPT_H

#include "interceptors.h"

#ifdef INTERCEPT
#define calloc(...)     CONCAT(INTERCEPTOR_PREFIX, calloc)(__VA_ARGS__)
#define malloc(...)     CONCAT(INTERCEPTOR_PREFIX, malloc)(__VA_ARGS__)
#define realloc(...)    CONCAT(INTERCEPTOR_PREFIX, realloc)(__VA_ARGS__)
#define socket(...)     CONCAT(INTERCEPTOR_PREFIX, socket)(__VA_ARGS__)
#define recv(...)       CONCAT(INTERCEPTOR_PREFIX, recv)(__VA_ARGS__)
#define setsockopt(...) CONCAT(INTERCEPTOR_PREFIX, setsockopt)(__VA_ARGS__)
#define listen(...)     CONCAT(INTERCEPTOR_PREFIX, listen)(__VA_ARGS__)
#define bind(...)       CONCAT(INTERCEPTOR_PREFIX, bind)(__VA_ARGS__)
#define accept(...)     CONCAT(INTERCEPTOR_PREFIX, accept)(__VA_ARGS__)
#endif /* INTERCEPT */

#define EXPAND_INTERCEPTOR(return_type, function_name, ...)     \
  return_type CONCAT(INTERCEPTOR_PREFIX, function_name)(GET_TYPES(__VA_ARGS__));
#include "interceptors.def"

#endif /* INTERCEPT_H */
