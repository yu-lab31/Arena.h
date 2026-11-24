#ifndef UNIT_TEST_ASSERT__
#define UNIT_TEST_ASSERT__

#include "unit_test/unit_test_log.h"

#define UT_ASSERT_BUFFER_SZ (100)

#define ASSERT(EXP, format, ...)                                               \
    do {                                                                       \
        int res = (EXP);                                                       \
        if (res)                                                               \
            continue;                                                          \
        char msg[UT_ASSERT_BUFFER_SZ];                                         \
        snprintf(msg, UT_ASSERT_BUFFER_SZ, format, ##__VA_ARGS__);             \
        UT_FATAL("assertion failed at (%s:%u) %s: %s", __FILE__, __LINE__,     \
                 __func__, msg);                                               \
        assert(0);                                                             \
    } while (0)

#define ASSERT_ThrSafe(EXP, ret_val, format, ...)                              \
    do {                                                                       \
        int res = (EXP);                                                       \
        if (res)                                                               \
            continue;                                                          \
        char msg[UT_ASSERT_BUFFER_SZ];                                         \
        snprintf(msg, UT_ASSERT_BUFFER_SZ, format, ##__VA_ARGS__);             \
        UT_FATAL("assertion failed at (%s:%u) %s: %s", __FILE__, __LINE__,     \
                 __func__, msg);                                               \
        pthread_exit((void *)ret_val);                                         \
    } while (0)

#endif // UNIT_TEST_ASSERT__
