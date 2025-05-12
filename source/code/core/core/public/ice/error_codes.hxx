/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/error.hxx>

namespace ice
{

    static constexpr ice::ErrorCodeSuccess S_Ok{ "S.0000:General:Success" };
    static constexpr ice::ErrorCodeSuccess S_Success = S_Ok;

    static constexpr ice::ErrorCodeError E_Fail{ "E.0001:General:Unknown error" };
    static constexpr ice::ErrorCodeError E_Error = E_Fail;

    static constexpr ice::ErrorCode E_InvalidArgument{ "E.0002:General:Invalid argument provided" };
    static constexpr ice::ErrorCode E_OutOfRange{ "E.0003:General:Accessing value out of range" };
    static constexpr ice::ErrorCode E_NotImplemented{ "E.0004:General:Function or method is not implemented" };
    static constexpr ice::ErrorCode E_TaskCanceled{ "E.1001:Tasks:Task canceled" };

    // Aliases are comparable
    static_assert(S_Ok == S_Success);
    static_assert(E_Fail == E_Error);

    // Success is not an error
    static_assert(S_Success != E_Error);

    // Generic Error comparable with specific error but not vice versa.
    static_assert(E_Error == E_InvalidArgument);
    static_assert(E_TaskCanceled != E_InvalidArgument);

} // namespace ice
