/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/error.hxx>

namespace ice
{

    static constexpr ice::ErrorCode S_Ok{ "S.0000:General:Success" };
    static constexpr ice::ErrorCode S_Success = S_Ok;

    static constexpr ice::ErrorCode E_Fail{ "E.0001:General:Unknown error" };
    static constexpr ice::ErrorCode E_Error = E_Fail;

    static constexpr ice::ErrorCode E_InvalidArgument{ "E.0002:General:Invalid argument provided" };
    static constexpr ice::ErrorCode E_OutOfRange{ "E.0003:General:Accessing value out of range" };
    static constexpr ice::ErrorCode E_TaskCanceled{ "E.1001:Tasks:Task canceled" };

} // namespace ice
