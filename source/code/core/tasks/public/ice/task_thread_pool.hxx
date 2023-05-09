#pragma once
#include <ice/task_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string_types.hxx>
#include <ice/span.hxx>

namespace ice
{

    struct TaskThreadPoolCreateInfo
    {
        //! \brief The thread count of this thread pool.
        ice::ucount thread_count = 0;

        //! \brief May be ignored in some builds.
        ice::String debug_name_format = "ice.thread {}";
    };

    class TaskThreadPool
    {
    public:
        virtual ~TaskThreadPool() noexcept = default;
        virtual auto thread_count() const noexcept -> ice::ucount = 0;
        virtual auto managed_thread_count() const noexcept -> ice::ucount = 0;
        virtual auto estimated_task_count() const noexcept -> ice::ucount = 0;

        //! \brief Creates an additonal thread with the given name (ID).
        //!
        //! \note This allows you to go over the initial thread count.
        virtual auto create_thread(ice::StringID name) noexcept -> ice::TaskThread& = 0;

        //! \brief Finds a thread created or attached with the given name.
        //!
        //! \note Default created threads cannot be found using this function.
        virtual auto find_thread(ice::StringID name) noexcept -> ice::TaskThread* = 0;

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
            //ice::TaskFlags accepting_flags,
            ice::UniquePtr<ice::TaskThread> thread
        ) noexcept -> ice::TaskThread& = 0;

        //! \brief Detaches a previously user created thread from the pool.
        //!
        //! \note This function does not detach created named threads.
        virtual auto detach_thread(
            ice::StringID name
        ) noexcept -> ice::UniquePtr<ice::TaskThread> = 0;
    };

    auto create_thread_pool(
        ice::Allocator& alloc,
        ice::TaskQueue& queue,
        ice::TaskThreadPoolCreateInfo const& threadpool_info
    ) noexcept -> ice::UniquePtr<ice::TaskThreadPool>;

} // namespace ice
