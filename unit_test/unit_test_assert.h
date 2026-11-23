#ifndef UTIL_TEST_ASSERT__
#define UTIL_TEST_ASSERT__

#include "unit_test/unit_test_log.h"

#define ASSERT(EXP, MSG)                                                       \
    do {                                                                       \
        int res = EXP;                                                         \
        if (res)                                                               \
            continue;                                                          \
        UT_FATAL("assertion failed at (%s:%u) %s: %s", __FILE__, __LINE__,     \
                 __func__, MSG);                                               \
        assert(0);                                                             \
    } while (0)

#endif // UTIL_TEST_ASSERT__
