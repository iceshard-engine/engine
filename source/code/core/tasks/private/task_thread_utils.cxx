/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/task_thread_utils.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>

namespace ice::current_thread
{

    void sleep(Tms ms) noexcept
    {
#if ISP_WINDOWS
        ICE_ASSERT_CORE(ms.value <= ice::u32_max);
        SleepEx(DWORD(ms.value), 0);
#elif ISP_UNIX
        Tus const us = ms;
        usleep(us.value);
#else
        ICE_ASSERT_CORE(false);
#endif
    }

} // namespace ice::current_thread
