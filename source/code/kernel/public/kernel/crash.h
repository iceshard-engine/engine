#pragma once
#include "assert/assert.h"

// Constant to fail assertions
constexpr bool FailAlways = false;

#ifdef _DEBUG

#define SMART_ASSERT(cond) \
    do \
    { \
        if (!(cond)) \
        { \
            static assert::SmartAssert assert; \
            if (assert.report_failure(__FILE__, __LINE__, #cond, 0)) \
                DEBUG_BREAK(); \
        } \
    } while(0)
#define SMART_ASSERT_MSG(cond, message, ...) \
    do \
    { \
        if (!(cond)) \
        { \
            static assert::SmartAssert assert; \
            if (assert.report_failure(__FILE__, __LINE__, #cond, message, ##__VA_ARGS__)) \
                DEBUG_BREAK(); \
        } \
    } while(0)


#ifdef _DEBUG

#define CHKD(cond) SMART_ASSERT(cond)
#define CHKID(cond, message, ...) SMART_ASSERT_MSG(cond, message, ##__VA_ARGS__)

#define CHKRD(cond) SMART_ASSERT(cond)
#define CHKIRD(cond, message, ...) SMART_ASSERT_MSG(cond, message, ##__VA_ARGS__)

#elif defined RDEBUG

#define CHKRD(cond) SMART_ASSERT(cond)
#define CHKIRD(cond, message, ...) SMART_ASSERT_MSG(cond, message, ##__VA_ARGS__)

#elif defined NDEBUG

#define SMART_ASSERT(cond) \
    do { ASSERT_UNUSED(cond); } while (0)
#define SMART_ASSERT_MSG(cond, msg, ...) \
    do { ASSERT_UNUSED(cond); ASSERT_UNUSED(msg); } while (0)
#define CHKRD(cond) SMART_ASSERT(cond)
#define CHKIRD(cond, message, ...) SMART_ASSERT_MSG(cond, message, ##__VA_ARGS__)

#endif

#else

#define SMART_ASSERT(cond) \
    do { ASSERT_UNUSED(cond); } while (0)
#define SMART_ASSERT_MSG(cond, msg, ...) \
    do { ASSERT_UNUSED(cond); ASSERT_UNUSED(msg); } while (0)
#define CHKRD(cond) SMART_ASSERT(cond)
#define CHKIRD(cond, message, ...) SMART_ASSERT_MSG(cond, message, ##__VA_ARGS__)

#endif
