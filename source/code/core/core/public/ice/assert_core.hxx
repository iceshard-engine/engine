#pragma once
#include <ice/build/platform.hxx>

#include <assert.h>
#include <cassert>
#undef assert

// TODO: Move to different header
#if ISP_WINDOWS

#define ICE_PREPROC_ARG(a) a

//#define ICE_ASSERT_CORE(expression, message) \
//    (void)( \
//        (!!(expression)) || \
//        (_wassert( \
//            _CRT_WIDE(#expression "\n" ICE_PREPROC_ARG(__FILE__) "(" _CRT_STRINGIZE(__LINE__) "): " message) \
//            , nullptr, 0 \
//        ), 0) \
//    )

#define ICE_ASSERT_CORE(expression) (void)(                                              \
        (!!(expression)) ||                                                              \
        (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
    )


#else
#error "Invalid!"
#endif
