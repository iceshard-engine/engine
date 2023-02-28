#pragma once
#include <ice/task_types_v3.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string_types.hxx>
#include <ice/span.hxx>

namespace ice
{

    struct TaskThreadPoolInfo_v3
    {
        //! \brief The thread count of this thread pool.
        ice::ucount thread_count = 0;

        //! \brief If not set, creates a single queue for all task types.
        ice::Span<ice::TaskFlags> queues = { };

        //! \brief The queue that should be returned if flags didn't match any other queues.
        //!
        //! \note If '-1' no queue will be returned.
        ice::icount missing_flags_queue = -1;

        //! \brief Does the thread pool allow to attach additional threads.
        bool allow_attaching = true;

        //! \brief May be ignored in some builds.
        ice::String debug_name_format;
    };

    class TaskThreadPool_v3
    {
    public:
        virtual ~TaskThreadPool_v3() noexcept = default;
        virtual auto thread_count() const noexcept -> ice::ucount = 0;
        virtual auto managed_thread_count() const noexcept -> ice::ucount = 0;
        virtual auto estimated_task_count() const noexcept -> ice::ucount = 0;

        //! \brief Creates an additonal thread with the given name (ID).
        //!
        //! \note This allows you to go over the initial thread count.
        virtual auto create_thread(ice::StringID name) noexcept -> ice::TaskThread_v3& = 0;

        //! \brief Finds a thread created or attached with the given name.
        //!
        //! \note Default created threads cannot be found using this function.
        virtual auto find_thread(ice::StringID name) noexcept -> ice::TaskThread_v3* = 0;

        //! \brief Destroyes a previosuly created or attached thread with the given name.
        //!
        //! \note Default created threads cannot be destroyed using this function.
        virtual bool destroy_thread(ice::StringID name) noexcept = 0;

        //! \brief Attaches a user created thread to the pool.
        //!
        //! \note This allows the user to create a thread with a separate queue, which can be used to also process tasks queued to the pool.
        //!     In such a case the thread queue will receive a task poped from the poll queue.
        //! \note Only tasks matching the 'accepting_flags' mask will be pushed to this thread.
        //! \note The attached threads lifetime is managed by the pool after attaching.
        virtual auto attach_thread(
            ice::StringID name,
            ice::TaskFlags accepting_flags,
            ice::UniquePtr<ice::TaskThread_v3> thread
        ) noexcept -> ice::TaskThread_v3& = 0;

        //! \brief Detaches a previously user created thread from the pool.
        //!
        //! \note This function does not detach created named threads.
        virtual auto detach_thread(
            ice::StringID name
        ) noexcept -> ice::UniquePtr<ice::TaskThread_v3> = 0;

        ////! \brief Returns the default queue pusher for the given task flags.
        ////!
        ////! \note If no queues exists that can accept such flags it will return a nullptr.
        //virtual auto queue_pusher(
        //    ice::TaskFlags flags
        //) const noexcept -> ice::TaskQueuePusher* = 0;
    };

    auto create_thread_pool(
        ice::Allocator& alloc,
        ice::TaskQueue_v3& queue,
        ice::TaskThreadPoolInfo_v3 const& threadpool_info
    ) noexcept -> ice::UniquePtr<ice::TaskThreadPool_v3>;

} // namespace ice
