#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/pod/hash.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/task.hxx>
#include <ice/task_list.hxx>
#include <ice/assert.hxx>

#include <ice/memory/stack_allocator.hxx>

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
        // #TODO: The '_tasks' array is not thread safe and the game may crash here when at the same time this list
        // is moved in the `resume_on_start_stage` on the graphics thread.
        _tasks.push_back(ice::move(task));
    }

    void IceGfxTaskFrame::query_operations(ice::Span<ice::StringID_Hash const> names, ice::Span<GfxCmdOperation*> operation_heads) noexcept
    {
        ICE_ASSERT(
            ice::size(names) == ice::size(operation_heads),
            "Out arrays have different sizes! {} != {}",
            ice::size(names), ice::size(operation_heads)
        );

        ice::u32 const queue_count = ice::size(names);
        if (queue_count == 0)
        {
            return;
        }

        auto get_queue_index = [&](ice::StringID_Hash name) noexcept -> ice::u32
        {
            ice::u32 idx = 0;
            while(idx < queue_count && names[idx] != name)
            {
                idx += 1;
            }
            return idx;
        };

        GfxCmdOperation* operation = _task_head_commands.load(std::memory_order::acquire);
        while(operation != nullptr)
        {
            auto* next_operation = operation->_next;

            ice::u32 const idx = get_queue_index(operation->_queue_name);
            ICE_ASSERT(
                idx < queue_count,
                "Queue with name {} was not found for scheduled operation.",
                ice::hash(operation->_queue_name)
            );

            operation->_next = operation_heads[idx];
            operation_heads[idx] = operation;

            operation = next_operation;
        }
    }

    auto IceGfxTaskFrame::frame_start() noexcept -> ice::gfx::GfxFrameStartOperation
    {
        return GfxFrameStartOperation{ *this };
    }

    auto IceGfxTaskFrame::frame_commands(ice::StringID_Arg queue_name) noexcept -> ice::gfx::GfxFrameCommandsOperation
    {
        return GfxFrameCommandsOperation{ *this, queue_name };
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

    bool IceGfxFrame::query_queue_stages(
        ice::StringID_Arg queue_name,
        ice::pod::Array<ice::gfx::GfxStage*>& out_stages
    ) noexcept
    {
        ice::gfx::GfxPass* const pass = ice::pod::hash::get(_enqueued_passes, ice::hash(queue_name), nullptr);
        if (pass == nullptr)
        {
            return false;
        }

        ice::memory::StackAllocator_1024 alloc;
        ice::pod::Array<ice::StringID_Hash> stage_order{ alloc };
        pass->query_stage_order(stage_order);

        if (ice::pod::array::empty(stage_order))
        {
            return false;
        }

        bool has_all = true;
        for (ice::StringID_Hash stage_id : stage_order)
        {
            has_all &= ice::pod::hash::has(_stages, ice::hash(stage_id));
        }

        if (has_all)
        {
            ice::pod::array::reserve(out_stages, ice::size(stage_order));

            for (ice::StringID_Hash stage_id : stage_order)
            {
                ice::gfx::GfxStage const* stage = ice::pod::hash::get(
                    _stages,
                    ice::hash(stage_id),
                    ice::gfx::GfxStageSlot{ }
                ).stage;

                ice::pod::array::push_back(out_stages, stage);
            }
        }

        return has_all;
    }

    void IceGfxFrame::execute_passes(
        ice::EngineFrame const& frame,
        ice::gfx::IceGfxQueue& queue
    ) noexcept
    {
        //for (auto const& entry : _enqueued_passes)
        //{
        //    queue.execute_pass(frame, entry.value, _stages);
        //}

        ice::pod::hash::clear(_enqueued_passes);
    }

    auto create_pass_executor(
        ice::Allocator& alloc,
        ice::render::RenderFence const& fence,
        ice::render::QueueFlags flags,
        ice::gfx::IceGfxQueueGroup& queue_group,
        ice::gfx::IceGfxFrame& gfx_frame,
        ice::Span<ice::StringID_Hash> names,
        ice::Span<ice::gfx::GfxCmdOperation*> operations
    ) noexcept -> ice::UniquePtr<IceGfxPassExecutor>
    {
        ice::gfx::IceGfxQueue* queue = nullptr;
        if (queue_group.get_queue(flags, queue) == false)
        {
            return ice::make_unique_null<IceGfxPassExecutor>();
        }

        GfxCmdOperation* operation_list = nullptr;

        ice::u32 idx = 0;
        for (; idx < ice::size(names); ++idx)
        {
            if (names[idx] == ice::stringid_hash(queue->name()))
            {
                operation_list = operations[idx];
                break;
            }
        }

        ice::pod::Array<ice::gfx::GfxStage*> stages{ alloc };

        bool const has_stages = gfx_frame.query_queue_stages(queue->name(), stages);
        bool const has_operations = operation_list != nullptr;

        if ((has_operations || has_stages) == false)
        {
            return ice::make_unique_null<IceGfxPassExecutor>();
        }

        if (has_operations && has_stages)
        {
            return ice::make_unique<IceGfxPassExecutor>(
                alloc,
                fence,
                queue,
                operation_list,
                ice::pod::Array<ice::gfx::GfxStage*>{ ice::memory::null_allocator() }
            );
        }

        return ice::make_unique<IceGfxPassExecutor>(
            alloc,
            fence,
            queue,
            operation_list,
            ice::move(stages)
        );
    }

    IceGfxPassExecutor::IceGfxPassExecutor(
        ice::render::RenderFence const& fence,
        ice::gfx::IceGfxQueue* queue,
        ice::gfx::GfxCmdOperation* operations,
        ice::pod::Array<ice::gfx::GfxStage*> stages
    ) noexcept
        : _fence{ fence }
        , _queue{ queue }
        , _operations{ operations }
        , _stages{ ice::move(stages) }
    {
        using ice::render::CommandBufferType;
        _queue->request_command_buffers(CommandBufferType::Primary, { &_command_buffer, 1 });
    }

    void IceGfxPassExecutor::record(ice::EngineFrame const& frame, ice::render::RenderCommands& api) noexcept
    {
        _api = &api;
        if (_operations != nullptr)
        {
            api.begin(_command_buffer);
            while (_operations != nullptr)
            {
                auto* next_operation = _operations->_next;
                _operations->_commands = this;
                _operations->_coroutine.resume();
                _operations = next_operation;
            }
            api.end(_command_buffer);
        }
        _api = nullptr;

        for (ice::gfx::GfxStage* stage : _stages)
        {
            stage->record_commands(frame, _command_buffer, api);
        }
    }

    void IceGfxPassExecutor::execute() noexcept
    {
        _queue->submit_command_buffers({ &_command_buffer, 1 }, &_fence);
    }

    //void IceGfxPassExecutor::wait() const noexcept
    //{
    //}

    void IceGfxPassExecutor::update_texture(
        ice::render::Image image,
        ice::render::Buffer image_contents,
        ice::vec2u extents
    ) noexcept
    {
        _api->update_texture(_command_buffer, image, image_contents, extents);
    }

} // namespace ice::gfx
