/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once

////! \brief This macro is required for a bug apprearing in the MSVC compiler when generating optimized code with '/O2'.
////!     It affects coroutine functions / methods and seems to mainly occur when using loops in the function body.
////!     A workaround for this problem is to separate the logic of such a coroutine into a separate function and mark it as 'noinline'
////!     Once the bug is fixed this macro should be removed.
////!
////! GitHub Issue: #108
//#define ISATTR_NOINLINE
//
//#if ISP_WINDOWS
//
//#undef ISATTR_NOINLINE
//#define ISATTR_NOINLINE __declspec(noinline)
//
//#endif // #if ISP_WINDOWS
