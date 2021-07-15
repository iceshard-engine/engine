#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
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
        } while (
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
        _task_executor = ice::IceshardTaskExecutor{ _allocator, ice::move(_tasks) };
        _task_executor.start_all();

        ice::pod::Array<ice::detail::ScheduleOperationData*> operations{ _allocator };

        ice::detail::ScheduleOperationData* operation = _task_head_start.load();
        while (operation != nullptr)
        {
            auto* next_operation = operation->_next;
            ice::pod::array::push_back(operations, operation);
            operation = next_operation;
        }

        ice::i32 const count = static_cast<ice::i32>(ice::pod::array::size(operations));
        for (ice::i32 idx = count - 1; idx >= 0; --idx)
        {
            operations[idx]->_coroutine.resume();
        }
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

    void IceGfxTaskFrame::execute_final_tasks() noexcept
    {
        _task_executor = ice::IceshardTaskExecutor{ _allocator, ice::move(_tasks) };
        _task_executor.start_all();
        _task_executor.wait_ready();
    }

    IceGfxFrame::IceGfxFrame(
        ice::Allocator& alloc
    ) noexcept
        : ice::gfx::IceGfxTaskFrame{ alloc }
        , _allocator{ alloc }
        , _enqueued_passes{ _allocator }
        , _queue_group{ nullptr }
        , _stages{ _allocator }
    {
    }

    void IceGfxFrame::set_stage_slot(
        ice::gfx::GfxStageSlot slot
    ) noexcept
    {
        if (slot.stage != nullptr)
        {
            set_stage_slots({ &slot, 1 });
        }
    }


    void IceGfxFrame::set_stage_slots(
        ice::Span<ice::gfx::GfxStageSlot const> slots
    ) noexcept
    {
        for (ice::gfx::GfxStageSlot const& slot : slots)
        {
            ice::pod::hash::set(
                _stages,
                ice::hash(slot.name),
                slot
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

        for (auto const& entry : _enqueued_passes)
        {
            ice::gfx::IceGfxQueue* const queue = queue_group.get_queue(ice::StringID{ StringID_Hash{ entry.key } });
            if (queue != nullptr)
            {
                queue->execute_pass(frame, entry.value, _stages);
            }
        }

        ice::pod::hash::clear(_enqueued_passes);
    }

} // namespace ice::gfx
