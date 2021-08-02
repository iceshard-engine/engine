#pragma once
#include <ice/engine_frame.hxx>
#include <ice/entity/entity_command_buffer.hxx>

#include <ice/input/input_event.hxx>
#include <ice/task.hxx>

#include <ice/memory/scratch_allocator.hxx>
#include <ice/collections.hxx>
#include <ice/pod/array.hxx>

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

        auto index() const noexcept -> ice::u32 override;

        auto allocator() noexcept -> ice::Allocator& override;
        auto memory_consumption() noexcept -> ice::u32 override;

        auto input_events() noexcept -> ice::pod::Array<ice::input::InputEvent>&;
        auto input_events() const noexcept -> ice::Span<ice::input::InputEvent const> override;

        void execute_task(ice::Task<void> task) noexcept;
        void start_all() noexcept;
        void wait_ready() noexcept;

        auto shards() const noexcept -> ice::Span<ice::Shard const> override;
        void push_shards(ice::Span<ice::Shard const> shards) noexcept override;

        auto entity_commands() noexcept -> ice::EntityCommandBuffer& override;

        auto entity_commands() const noexcept -> ice::EntityCommandBuffer const&;

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

        auto schedule_frame_end() noexcept -> ice::FrameEndOperation override;

    protected:
        void schedule_internal(
            ice::FrameEndOperationData& operation
        ) noexcept override;

    private:
        ice::u32 const _index;
        ice::memory::ScratchAllocator& _allocator;
        ice::memory::ScratchAllocator _inputs_allocator;
        ice::memory::ScratchAllocator _request_allocator;
        ice::memory::ScratchAllocator _tasks_allocator;
        ice::memory::ScratchAllocator _storage_allocator;
        ice::memory::ScratchAllocator _data_allocator;

        ice::pod::Array<ice::input::InputEvent> _input_events;
        ice::pod::Array<ice::Shard> _shards;
        ice::EntityCommandBuffer _entity_commands;
        ice::pod::Hash<void*> _named_objects;

        ice::Vector<ice::Task<>> _frame_tasks;
        ice::IceshardTaskExecutor _task_executor;

        std::atomic<ice::FrameEndOperationData*> _frame_end_operation;
    };

} // namespace ice
