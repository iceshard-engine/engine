#pragma once
#include <iceshard/frame.hxx>

#include <core/allocator.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>
#include <core/clock.hxx>

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
            iceshard::IceshardExecutionInstance& execution_instance
        ) noexcept;
        ~MemoryFrame() noexcept;

        auto engine() noexcept -> Engine& override;

        auto execution_instance() const noexcept -> IceshardExecutionInstance const&
        {
            return _execution_instance;
        }

        auto messages() noexcept -> core::MessageBuffer& { return _frame_messages; }

        auto messages() const noexcept -> core::MessageBuffer const& override;

        auto input_queue() noexcept -> iceshard::input::DeviceInputQueue& { return _input_queue; }

        auto input_queue() const noexcept -> iceshard::input::DeviceInputQueue const& override;

        auto input_events() noexcept -> core::pod::Array<iceshard::input::InputEvent>& { return _input_events; }

        auto input_events() const noexcept -> core::pod::Array<iceshard::input::InputEvent> const& override;

        auto input_actions() noexcept -> core::pod::Array<core::stringid_type>& { return _input_actions; }

        auto input_actions() const noexcept -> core::pod::Array<core::stringid_type> const& override;

        auto find_frame_object(core::stringid_arg_type name) noexcept -> void* override;

        auto find_frame_object(core::stringid_arg_type name) const noexcept -> const void* override;

        void add_frame_object(core::stringid_arg_type name, void* frame_object, void(*deleter)(core::allocator&, void*)) noexcept override;

        auto frame_allocator() noexcept -> core::allocator& override;

        auto engine_clock() const noexcept -> core::Clock const& override;

        auto elapsed_time() const noexcept -> float override;

        void add_task(cppcoro::task<> task) noexcept override;

    private:
        core::memory::scratch_allocator& _frame_allocator;
        iceshard::Engine& _engine;
        iceshard::IceshardExecutionInstance& _execution_instance;

        core::memory::scratch_allocator _inputs_allocator;
        core::memory::scratch_allocator _message_allocator;
        core::memory::scratch_allocator _storage_allocator;
        core::memory::scratch_allocator _data_allocator;

        iceshard::input::DeviceInputQueue _input_queue;
        core::pod::Array<iceshard::input::InputEvent> _input_events;
        core::pod::Array<core::stringid_type> _input_actions;

        core::MessageBuffer _frame_messages;

        struct frame_object_entry
        {
            void* object_instance;
            void(*object_deleter)(core::allocator&, void*);
        };
        core::pod::Hash<frame_object_entry> _frame_storage;
    };


} // namespace iceshard
