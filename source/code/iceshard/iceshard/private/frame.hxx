#pragma once
#include <iceshard/frame.hxx>

#include <core/allocator.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>

namespace iceshard
{

    class IceshardExecutionInstance;

    //! \brief A single engine frame with a preallocated ring buffer allocator.
    class MemoryFrame final : public iceshard::Frame
    {
    public:
        MemoryFrame(
            core::memory::scratch_allocator& alloc,
            iceshard::Engine& engine,
            iceshard::IceshardExecutionInstance& execution_instance,
            float time_delta
        ) noexcept;
        ~MemoryFrame() noexcept;

        auto engine() noexcept -> Engine& override;

        auto messages() noexcept -> core::MessageBuffer& { return _frame_messages; }

        auto messages() const noexcept -> core::MessageBuffer const& override;

        auto input_queue() noexcept -> iceshard::input::DeviceInputQueue& { return _input_queue; }

        auto input_queue() const noexcept -> iceshard::input::DeviceInputQueue const& override;

        auto input_events() noexcept -> core::pod::Array<iceshard::input::InputEvent>& { return _input_events; }

        auto input_events() const noexcept -> core::pod::Array<iceshard::input::InputEvent> const& override;

        auto find_frame_object(core::stringid_arg_type name) noexcept -> void* override;

        auto find_frame_object(core::stringid_arg_type name) const noexcept -> const void* override;

        void add_frame_object(core::stringid_arg_type name, void* frame_object, void(*deleter)(core::allocator&, void*)) noexcept override;

        auto frame_allocator() noexcept -> core::allocator& override;

        auto time_delta() const noexcept -> float override
        {
            return _time_delta;
        }

        auto tick() const noexcept -> core::datetime::tick_type override
        {
            return _tick;
        }

        void add_task(cppcoro::task<> task) noexcept override;

    private:
        core::memory::scratch_allocator& _frame_allocator;
        iceshard::Engine& _engine;
        iceshard::IceshardExecutionInstance& _execution_instance;

        float const _time_delta;
        core::datetime::tick_type const _tick;

        core::memory::scratch_allocator _inputs_allocator;
        core::memory::scratch_allocator _message_allocator;
        core::memory::scratch_allocator _storage_allocator;
        core::memory::scratch_allocator _data_allocator;

        iceshard::input::DeviceInputQueue _input_queue;
        core::pod::Array<iceshard::input::InputEvent> _input_events;
        core::MessageBuffer _frame_messages;

        struct frame_object_entry
        {
            void* object_instance;
            void(*object_deleter)(core::allocator&, void*);
        };
        core::pod::Hash<frame_object_entry> _frame_storage;
    };


} // namespace iceshard
