#include <ice/task_thread_pool.hxx>
#include <ice/os/windows.hxx>

#if ISP_WINDOWS

namespace ice
{

    namespace detail
    {

        void threadpool_coroutine_work_callback(
            PTP_CALLBACK_INSTANCE instance,
            PVOID context,
            PTP_WORK work
        ) noexcept
        {
            ice::detail::ScheduleOperationData const* operation_data = reinterpret_cast<ice::detail::ScheduleOperationData*>(context);
            operation_data->_coroutine.resume();
            CloseThreadpoolWork(work);
        }

    } // namespace detail

    class IceTaskThreadPool : public ice::TaskThreadPool
    {
    public:
        IceTaskThreadPool(ice::u32 thread_count) noexcept
            : _thread_count{ thread_count }
            , _native_threadpool{ }
            , _native_tp_callback{ }
            , _native_tp_group_cleanup{ }
        {
            _native_threadpool = CreateThreadpool(nullptr);
            SetThreadpoolThreadMinimum(_native_threadpool, ice::min(_thread_count, 2u));
            SetThreadpoolThreadMaximum(_native_threadpool, _thread_count);

            InitializeThreadpoolEnvironment(&_native_tp_callback);
            SetThreadpoolCallbackPool(&_native_tp_callback, _native_threadpool);

            _native_tp_group_cleanup = CreateThreadpoolCleanupGroup();
            SetThreadpoolCallbackCleanupGroup(&_native_tp_callback, _native_tp_group_cleanup, nullptr);
        }

        ~IceTaskThreadPool() noexcept override
        {
            //CloseThreadpoolTimer(_native_tp_timer);
            CloseThreadpoolCleanupGroupMembers(_native_tp_group_cleanup, true, nullptr);
            CloseThreadpoolCleanupGroup(_native_tp_group_cleanup);
            CloseThreadpool(_native_threadpool);
        }

        void schedule_internal(
            ScheduleOperation* op,
            ScheduleOperation::DataMemberType data_member
        ) noexcept override
        {
            ice::detail::ScheduleOperationData& data = op->*data_member;
            SubmitThreadpoolWork(
                CreateThreadpoolWork(
                    detail::threadpool_coroutine_work_callback,
                    &data,
                    &_native_tp_callback
                )
            );
        }

    private:
        ice::u32 const _thread_count;
        PTP_POOL _native_threadpool;
        TP_CALLBACK_ENVIRON _native_tp_callback;
        PTP_CLEANUP_GROUP _native_tp_group_cleanup;

        std::atomic<ice::detail::ScheduleOperationData*> _head = nullptr;
    };

    auto create_simple_threadpool(
        ice::Allocator& alloc,
        ice::u32 thread_count
    ) noexcept -> ice::UniquePtr<ice::TaskThreadPool>
    {
        return ice::make_unique<ice::IceTaskThreadPool>(alloc, thread_count);
    }

} // namespace ice

#endif
