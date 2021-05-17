#include <ice/task_scheduler.hxx>

namespace ice
{

    ScheduleOperation::ScheduleOperation(ice::TaskScheduler& scheduler) noexcept
        : _scheduler{ scheduler }
        , _coro{ nullptr }
        , _next{ nullptr }
    {
    }

} // namespace ice
