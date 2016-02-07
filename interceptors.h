#ifndef INTERCEPTOR_PREFIX
#define INTERCEPTOR_PREFIX intercept_
#endif

#define CONCAT2(a, b) a ## b
#define CONCAT(a, b) CONCAT2(a, b)

#define INTERCEPT_FUNC(func_name) CONCAT(INTERCEPTOR_PREFIX, func_name)
#define INTERCEPT_IDX(func_name) CONCAT(INTERCEPTED_, func_name)

#define GET_TYPES_1(t1, v1)                     \
  t1 v1
#define GET_TYPES_2(t1, v1, t2, v2)             \
  t1 v1, t2 v2
#define GET_TYPES_3(t1, v1, t2, v2, t3, v3)     \
  t1 v1, t2 v2, t3 v3
#define GET_TYPES_4(t1, v1, t2, v2, t3, v3, t4, v4)     \
  t1 v1, t2 v2, t3 v3, t4 v4
#define GET_TYPES_5(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)     \
  t1 v1, t2 v2, t3 v3, t4 v4, t5 v5
#define GET_TYPES_6(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6)     \
  t1 v1, t2 v2, t3 v3, t4 v4, t5 v5, t6 v6
#define GET_TYPES_7(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7) \
  t1 v1, t2 v2, t3 v3, t4 v4, t5 v5, t6 v6, t7 v7

#define GET_VARS_1(t1, v1)                      \
  v1
#define GET_VARS_2(t1, v1, t2, v2)              \
  v1, v2
#define GET_VARS_3(t1, v1, t2, v2, t3, v3)      \
  v1, v2, v3
#define GET_VARS_4(t1, v1, t2, v2, t3, v3, t4, v4)      \
  v1, v2, v3, v4
#define GET_VARS_5(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)      \
  v1, v2, v3, v4, v5
#define GET_VARS_6(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6)      \
  v1, v2, v3, v4, v5, v6
#define GET_VARS_7(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7) \
  v1, v2, v3, v4, v5, v6, v7

#define OVERLOAD(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, NAME, ...) \
  NAME

#define GET_TYPES(...) OVERLOAD(__VA_ARGS__,                            \
                                GET_TYPES_7, UNBALANCED,                \
                                GET_TYPES_6, UNBALANCED,                \
                                GET_TYPES_5, UNBALANCED,                \
                                GET_TYPES_4, UNBALANCED,                \
                                GET_TYPES_3, UNBALANCED,                \
                                GET_TYPES_2, UNBALANCED,                \
                                GET_TYPES_1, UNBALANCED                 \
                                )(__VA_ARGS__)

#define GET_VARS(...) OVERLOAD(__VA_ARGS__,                            \
                               GET_VARS_7, UNBALANCED,                 \
                               GET_VARS_6, UNBALANCED,                 \
                               GET_VARS_5, UNBALANCED,                 \
                               GET_VARS_4, UNBALANCED,                 \
                               GET_VARS_3, UNBALANCED,                 \
                               GET_VARS_2, UNBALANCED,                 \
                               GET_VARS_1, UNBALANCED                  \
                               )(__VA_ARGS__)
