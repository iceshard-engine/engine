#pragma once
#include <ice/task.hxx>
#include <ice/engine_frame.hxx>
#include <ice/input/input_event.hxx>
#include <ice/memory/scratch_allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/collections.hxx>

#include "iceshard_task_executor.hxx"

namespace ice
{

    class IceshardMemoryFrame final : public ice::EngineFrame
    {
    public:
        IceshardMemoryFrame(
            ice::memory::ScratchAllocator& alloc
        ) noexcept;
        ~IceshardMemoryFrame() noexcept override;

        auto allocator() noexcept -> ice::Allocator& override;
        auto memory_consumption() noexcept -> ice::u32 override;

        auto input_events() noexcept -> ice::pod::Array<ice::input::InputEvent>&;
        auto input_events() const noexcept -> ice::Span<ice::input::InputEvent const> override;

        void execute_task(ice::Task<void> task) noexcept override;
        void start_all() noexcept;
        void wait_ready() noexcept;

        void push_requests(
            ice::Span<EngineRequest const> requests
        ) noexcept override;

        auto named_data(
            ice::StringID_Arg name
        ) noexcept -> void* override;

        auto named_data(
            ice::StringID_Arg name
        ) const noexcept -> void const* override;

        auto allocate_named_data(
            ice::StringID_Arg name,
            ice::u32 size,
            ice::u32 alignment
        ) noexcept -> void* override;

        void release_named_data(
            ice::StringID_Arg name
        ) noexcept override;

        auto requests() const noexcept -> ice::Span<EngineRequest const>;

    private:
        ice::memory::ScratchAllocator& _allocator;
        ice::memory::ScratchAllocator _inputs_allocator;
        ice::memory::ScratchAllocator _request_allocator;
        ice::memory::ScratchAllocator _tasks_allocator;
        ice::memory::ScratchAllocator _storage_allocator;
        ice::memory::ScratchAllocator _data_allocator;

        ice::pod::Array<ice::input::InputEvent> _input_events;
        ice::pod::Array<ice::EngineRequest> _requests;
        ice::pod::Hash<void*> _named_objects;

        ice::Vector<ice::Task<>> _frame_tasks;
        ice::IceshardTaskExecutor _task_executor;
    };

} // namespace ice
