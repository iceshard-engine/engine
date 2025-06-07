#include <ice/task_thread_utils.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>

namespace ice::current_thread
{

    void sleep(Tms ms) noexcept
    {
#if ISP_WINDOWS
        SleepEx(ms.value, 0);
#elif ISP_LINUX
        Tus const us = ms;
        usleep(us.value);
#endif
    }

} // namespace ice::current_thread
