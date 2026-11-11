#include <ice/mem_allocator_host.hxx>
#include <ice/semaphore.hxx>

namespace ice
{

#if ISP_UNIX
    detail::UnixSemaphore::~UnixSemaphore() noexcept
    {
        sem_destroy(&_native);
    }

    bool detail::UnixSemaphore::post() noexcept
    {
        return sem_post(&_native) == 0;
    }

    bool detail::UnixSemaphore::wait(ice::Tms duration) noexcept
    {
        timespec timeval;
        if (clock_gettime(CLOCK_REALTIME, &timeval) != 0)
        {
            return false;
        }

        ice::i64 const seconds = duration.value / Tms::Constant_Precision;
        timeval.tv_sec += seconds;
        duration.value -= seconds * Tms::Constant_Precision;

        ice::Tns const reminder = duration;
        timeval.tv_nsec += reminder.value;

        return sem_timedwait(&_native, &timeval) == 0;
    }

    auto create_semaphore(ice::Allocator& alloc, ice::i32 initial_value) noexcept -> ice::Semaphore
    {
        ice::Semaphore result = ice::make_unique<ice::detail::UnixSemaphore>(alloc);
        if (sem_init(ice::addressof(result->_native), 0, initial_value) != 0)
        {
            return {};
        }
        return result;
    }
#endif // #if ISP_UNIX

} // namespace ice