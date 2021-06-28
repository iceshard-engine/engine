#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include "iceshard_gfx_pass.hxx"
#include <ice/pod/hash.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/task.hxx>
#include <ice/task_list.hxx>
#include <ice/assert.hxx>

#include "../iceshard_task_executor.hxx"

namespace ice::gfx
{

    IceGfxTaskFrame::IceGfxTaskFrame(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _tasks{ _allocator }
        , _task_executor{ _allocator, ice::move(_tasks) }
    { }

    void IceGfxTaskFrame::execute_task(ice::Task<> task) noexcept
    {
        _tasks.push_back(ice::move(task));
    }

    auto IceGfxTaskFrame::frame_start() noexcept -> ice::gfx::GfxFrameStartOperation
    {
        return GfxFrameStartOperation{ *this };
    }

    auto IceGfxTaskFrame::frame_commands(ice::StringID_Arg queue_name) noexcept -> ice::gfx::GfxFrameCommandsOperation
    {
        return GfxFrameCommandsOperation{ *this, _task_commands, queue_name };
    }

    auto IceGfxTaskFrame::frame_end() noexcept -> ice::gfx::GfxFrameEndOperation
    {
        return GfxFrameEndOperation{ *this };
    }

    void IceGfxTaskFrame::schedule_internal(
        ice::gfx::GfxFrameStartOperation* operation,
        ice::gfx::GfxFrameStartOperation::DataMemberType data_member
    ) noexcept
    {
        ice::detail::ScheduleOperationData& data = operation->*data_member;
        ice::detail::ScheduleOperationData* expected_head = _task_head_start.load(std::memory_order_acquire);

        do
        {
            data._next = expected_head;
        }
        while (
            _task_head_start.compare_exchange_weak(
                expected_head,
                &data,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void IceGfxTaskFrame::schedule_internal(
        ice::gfx::GfxFrameCommandsOperation* operation,
        ice::gfx::GfxFrameCommandsOperation::DataMemberType data_member
    ) noexcept
    {
        ice::gfx::GfxFrameCommandsOperation::OperationData& data = operation->*data_member;
        ice::gfx::GfxFrameCommandsOperation::OperationData* expected_head = _task_head_commands.load(std::memory_order_acquire);

        do
        {
            data._next = expected_head;
        }
        while (
            _task_head_commands.compare_exchange_weak(
                expected_head,
                &data,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void IceGfxTaskFrame::schedule_internal(
        ice::gfx::GfxFrameEndOperation* operation,
        ice::gfx::GfxFrameEndOperation::DataMemberType data_member
    ) noexcept
    {
        ice::detail::ScheduleOperationData& data = operation->*data_member;
        ice::detail::ScheduleOperationData* expected_head = _task_head_end.load(std::memory_order_acquire);

        do
        {
            data._next = expected_head;
        }
        while (
            _task_head_end.compare_exchange_weak(
                expected_head,
                &data,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    void IceGfxTaskFrame::resume_on_start_stage() noexcept
    {
        ice::detail::ScheduleOperationData* operation = _task_head_start.load();
        while (operation != nullptr)
        {
            auto* next_operation = operation->_next;
            operation->_coroutine.resume();
            operation = next_operation;
        }

        _task_executor = ice::IceshardTaskExecutor{ _allocator, ice::move(_tasks) };
        _task_executor.start_all();
    }

    void IceGfxTaskFrame::resume_on_commands_stage(
        ice::StringID_Arg queue_name,
        ice::gfx::IceGfxQueue* queue
    ) noexcept
    {
        ICE_ASSERT(
            queue != nullptr,
            "Cannot resume `commands stage` without a valid `GfxQueue` object!"
        );

        // Update the context value for `_task_commands` as it will now be used by each resumed task!
        _task_commands = queue;

        // We can now safely resume all tasks.
        ice::gfx::GfxFrameCommandsOperation::OperationData* operation = _task_head_commands.load();
        if (operation == nullptr)
        {
            operation = _skipped_tasks_temporary;
            _skipped_tasks_temporary = nullptr;
        }

        queue->test_begin();
        while (operation != nullptr)
        {
            if (operation->queue_name == ice::stringid_hash(queue_name))
            {
                auto* next_operation = operation->_next;
                operation->_coroutine.resume();
                operation = next_operation;
            }
            else
            {
                auto* next_operation = operation->_next;

                operation->_next = _skipped_tasks_temporary;
                _skipped_tasks_temporary = operation;

                operation = next_operation;
            }
        }
        queue->test_end();
    }

    void IceGfxTaskFrame::resume_on_end_stage() noexcept
    {
        ice::detail::ScheduleOperationData* operation = _task_head_end.load();
        while (operation != nullptr)
        {
            auto* next_operation = operation->_next;
            operation->_coroutine.resume();
            operation = next_operation;
        }

        _task_executor.wait_ready();
    }


    IceGfxFrame::IceGfxFrame(
        ice::Allocator& alloc
    ) noexcept
        : ice::gfx::IceGfxTaskFrame{ alloc }
        , _allocator{ alloc, "gfx-frame" }
        , _enqueued_passes{ _allocator }
        //, _frame_tasks{ _allocator }
        , _queue_group{ nullptr }
        , _stages{ _allocator }
        //, _task_executor{ _allocator, ice::Vector<ice::Task<void>> { _allocator } }
    {
        //_frame_tasks.reserve(1024);
    }

    IceGfxFrame::~IceGfxFrame() noexcept
    {
        //{
        //    GfxFrame::GfxAwaitCommandsOperation* expected_initial_task = _head.load(std::memory_order_acquire);
        //    if (expected_initial_task != nullptr)
        //    {
        //        while (
        //            _head.compare_exchange_weak(
        //                expected_initial_task,
        //                nullptr,
        //                std::memory_order::acquire,
        //                std::memory_order::relaxed
        //            ) == false
        //            )
        //        {
        //            continue;
        //        }

        //        while (expected_initial_task != nullptr)
        //        {
        //            GfxFrame::GfxAwaitCommandsOperation* next_op = expected_initial_task->_next;
        //            expected_initial_task->_coro.destroy();
        //            expected_initial_task = next_op;
        //        }
        //    }
        //}
        //{
        //    GfxFrame::GfxAwaitFrameEnd* expected_initial_task = _frame_end_head.load(std::memory_order_acquire);
        //    if (expected_initial_task != nullptr)
        //    {
        //        while (
        //            _frame_end_head.compare_exchange_weak(
        //                expected_initial_task,
        //                nullptr,
        //                std::memory_order::acquire,
        //                std::memory_order::relaxed
        //            ) == false
        //            )
        //        {
        //            continue;
        //        }

        //        while (expected_initial_task != nullptr)
        //        {
        //            GfxFrame::GfxAwaitFrameEnd* next_op = expected_initial_task->_next;
        //            expected_initial_task->_coro.destroy();
        //            expected_initial_task = next_op;
        //        }
        //    }
        //}
    }

    //auto IceGfxFrame::aquire_task_commands(ice::StringID_Arg queue_name) noexcept -> GfxFrame::GfxAwaitCommandsOperation
    //{
    //    return GfxAwaitCommandsOperation{ *this, queue_name };
    //}

    //auto IceGfxFrame::frame_end() noexcept -> GfxAwaitFrameEnd
    //{
    //    return GfxAwaitFrameEnd{ *this };
    //}

    //void IceGfxFrame::execute_task(ice::Task<void> task) noexcept
    //{
    //    _frame_tasks.push_back(ice::move(task));
    //}

    void IceGfxFrame::start_all() noexcept
    {
        //_task_executor = IceshardTaskExecutor{ _allocator, ice::move(_frame_tasks) };
        //_task_executor.start_all();
    }

    void IceGfxFrame::wait_ready() noexcept
    {
        //GfxFrame::GfxAwaitFrameEnd* expected_initial_task = _frame_end_head.load(std::memory_order_acquire);
        //if (expected_initial_task != nullptr)
        //{
        //    while (
        //        _frame_end_head.compare_exchange_weak(
        //            expected_initial_task,
        //            nullptr,
        //            std::memory_order::acquire,
        //            std::memory_order::relaxed
        //        ) == false
        //    )
        //    {
        //        continue;
        //    }

        //    while (expected_initial_task != nullptr)
        //    {
        //        auto* next_op = expected_initial_task->_next;
        //        expected_initial_task->_coro.resume();
        //        expected_initial_task = next_op;
        //    }
        //}

        //_task_executor.wait_ready();
    }

    void IceGfxFrame::set_stage(
        ice::StringID_Arg name,
        ice::gfx::GfxStage* stage
    ) noexcept
    {
        if (stage != nullptr)
        {
            ice::pod::hash::set(
                _stages,
                ice::hash(name),
                IceGfxStage{
                    .name = name,
                    .stage = stage
                }
            );
        }
    }

    void IceGfxFrame::enqueue_pass(
        ice::StringID_Arg queue_name,
        ice::gfx::GfxPass* pass
    ) noexcept
    {
        ice::pod::multi_hash::insert(
            _enqueued_passes,
            ice::hash(queue_name),
            pass
        );
    }

    void IceGfxFrame::execute_passes(
        ice::EngineFrame const& frame,
        ice::gfx::IceGfxQueueGroup& queue_group
    ) noexcept
    {
        _queue_group = &queue_group;

        //GfxFrame::GfxAwaitCommandsOperation* expected_initial_task = _head.load(std::memory_order_acquire);
        //if (expected_initial_task != nullptr)
        //{
        //    while (
        //        _head.compare_exchange_weak(
        //            expected_initial_task,
        //            nullptr,
        //            std::memory_order::acquire,
        //            std::memory_order::relaxed
        //        ) == false
        //        )
        //    {
        //        continue;
        //    }

        //    ice::gfx::IceGfxQueue* const queue = queue_group.get_queue("default"_sid);
        //    if (queue != nullptr)
        //    {
        //        queue->test_begin();
        //        while (expected_initial_task != nullptr)
        //        {
        //            auto* next_op = expected_initial_task->_next;
        //            expected_initial_task->_coro.resume();
        //            //ICE_ASSERT(
        //            //    expected_initial_task->_coro.done(),
        //            //    "Coroutine not finished!"
        //            //);

        //            //expected_initial_task->_coro.destroy();
        //            expected_initial_task = next_op;
        //        }
        //        queue->test_end();
        //    }
        //}

        for (auto const& entry : _enqueued_passes)
        {
            //ice::gfx::IceGfxPass const* gfx_pass = static_cast<ice::gfx::IceGfxPass const*>();
            //if (gfx_pass->has_work() == false || gfx_pass->is_complete(_stages) == false)
            //{
            //    continue;
            //}

            ice::gfx::IceGfxQueue* const queue = queue_group.get_queue(ice::StringID{ StringID_Hash{ entry.key } });
            if (queue != nullptr)
            {
                queue->execute_pass(frame, entry.value, _stages);
            }
        }

        ice::pod::hash::clear(_enqueued_passes);
    }

    //auto IceGfxFrame::task_commands(
    //    ice::StringID_Arg queue_name
    //) noexcept -> ice::gfx::GfxTaskCommands&
    //{
    //    return *_queue_group->get_queue(queue_name);
    //}

} // namespace ice::gfx
