/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_frame.hxx"
#include "iceshard_gfx_queue.hxx"
#include <ice/container/hashmap.hxx>
#include <ice/render/render_swapchain.hxx>
#include <ice/task.hxx>
#include <ice/task_list.hxx>
#include <ice/assert.hxx>

#include <ice/mem_allocator_stack.hxx>

#include "../iceshard_task_executor.hxx"

namespace ice::gfx
{

    namespace detail
    {

        template<typename T>
        void util_schedule_internal(std::atomic<T*>& atomic, T& operation) noexcept
        {
            T* expected_head = atomic.load(std::memory_order::acquire);

            bool schedule_success = false;

            do
            {
                operation.next = expected_head;

                schedule_success = atomic.compare_exchange_weak(
                    expected_head,
                    &operation,
                    std::memory_order::release,
                    std::memory_order::acquire
                );
            } while (schedule_success == false);
        }

    } // namespace detail

    IceGfxFrame::IceGfxFrame(
        ice::Allocator& alloc
    ) noexcept
        : ice::gfx::GfxFrame{ }
        , _allocator{ alloc }
        , _tasks{ _allocator }
        , _passes{ _allocator }
        , _frame_stages{ _allocator }
        , _context_stages{ _allocator }
    {
    }

    void IceGfxFrame::add_task(ice::Task<> task) noexcept
    {
        ice::array::push_back(_tasks, ice::move(task));
    }

    void IceGfxFrame::set_stage_slot(
        ice::StringID_Arg stage_name,
        ice::gfx::GfxContextStage const* stage
    ) noexcept
    {
        ice::hashmap::set(
            _context_stages,
            ice::hash(stage_name),
            stage
        );
    }

    void IceGfxFrame::enqueue_pass(
        ice::StringID_Arg queue_name,
        ice::StringID_Arg pass_name,
        ice::gfx::GfxPass const* pass
    ) noexcept
    {
        ice::array::push_back(
            _passes,
            IceGfxPassEntry{
                .queue_name = queue_name,
                .pass_name = pass_name,
                .pass = pass,
            }
        );
    }

    bool IceGfxFrame::query_queue_stages(
        ice::Span<ice::StringID_Hash const> stage_order,
        ice::Span<ice::gfx::GfxContextStage const*> out_stages
    ) noexcept
    {
        bool has_all = true;
        for (ice::StringID_Hash stage_id : stage_order)
        {
            bool const has_stage = ice::hashmap::has(_context_stages, ice::hash(stage_id));
            ICE_LOG_IF(
                has_stage == false,
                ice::LogSeverity::Debug, ice::LogTag::Engine,
                "Missing stage with ID: {}",
                stage_id
            );
            has_all &= has_stage;
        }

        if (has_all)
        {
            ICE_ASSERT(ice::count(stage_order) == ice::count(out_stages), "Cannot query stages into output span. Sizes differ!");

            ice::u32 idx = 0;
            for (ice::StringID_Hash stage_id : stage_order)
            {
                ice::gfx::GfxContextStage const* stage = ice::hashmap::get(
                    _context_stages,
                    ice::hash(stage_id),
                    nullptr
                );

                out_stages[idx] = stage;
                idx += 1;
            }
        }

        return has_all;
    }

    auto IceGfxFrame::enqueued_passes() const noexcept -> ice::Span<ice::gfx::IceGfxPassEntry const>
    {
        return _passes;
    }

    auto IceGfxFrame::create_task_executor() noexcept -> ice::IceshardTaskExecutor
    {
        return ice::IceshardTaskExecutor{ _allocator.backing_allocator(), ice::move(_tasks)};
    }

    void IceGfxFrame::on_frame_begin() noexcept
    {
        ice::gfx::detail::GfxTaskOperationData* operation = _operations_frame_begin.load();
        while (operation != nullptr)
        {
            ice::gfx::detail::GfxTaskOperationData* next_operation = operation->next;
            operation->coroutine.resume();
            operation = next_operation;
        }
    }

    bool IceGfxFrame::on_frame_stage(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer command_buffer,
        ice::render::RenderCommands& render_commands
    ) noexcept
    {

        ice::gfx::GfxAwaitExecuteStageData* operation = _operations_stage_execute.load();
        bool const result = operation != nullptr;

        if (result)
        {
            render_commands.begin(command_buffer);
            while (operation != nullptr)
            {
                ice::gfx::GfxAwaitExecuteStageData* next_operation = operation->next;
                operation->stage->record_commands(frame, command_buffer, render_commands);
                operation->coroutine.resume();
                operation = next_operation;
            }
            render_commands.end(command_buffer);
        }

        return result;
    }

    void IceGfxFrame::on_frame_end() noexcept
    {
        ice::gfx::detail::GfxTaskOperationData* operation = _operations_frame_end.load();
        while (operation != nullptr)
        {
            ice::gfx::detail::GfxTaskOperationData* next_operation = operation->next;
            operation->coroutine.resume();
            operation = next_operation;
        }
    }

    void IceGfxFrame::schedule_internal(
        ice::gfx::GfxAwaitBeginFrameData& operation
    ) noexcept
    {
        detail::util_schedule_internal(_operations_frame_begin, operation);
    }

    void IceGfxFrame::schedule_internal(
        ice::gfx::GfxAwaitExecuteStageData& operation
    ) noexcept
    {
        ICE_ASSERT(
            operation.stage != nullptr,
            "Cannot schedule a stage operation without a valid stage object!"
        );

        detail::util_schedule_internal(_operations_stage_execute, operation);
    }

    void IceGfxFrame::schedule_internal(
        ice::gfx::GfxAwaitEndFrameData& operation
    ) noexcept
    {
        detail::util_schedule_internal(_operations_frame_end, operation);
    }

} // namespace ice::gfx
