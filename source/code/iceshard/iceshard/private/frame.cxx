#include "frame.hxx"

#include <core/debug/assert.hxx>
#include <core/message/operations.hxx>
#include <core/datetime/datetime.hxx>
#include <core/pod/hash.hxx>

#include "iceshard_execution_instance.hxx"

namespace iceshard
{

    namespace detail
    {
        static constexpr uint32_t MiB = 1024 * 1024;
        static constexpr uint32_t inputs_allocator_pool = 4 * MiB;
        static constexpr uint32_t message_allocator_pool = 20 * MiB;
        static constexpr uint32_t storage_allocator_pool = 8 * MiB;
        static constexpr uint32_t data_allocator_pool = 196 * MiB;

        static uint32_t next_frame_index = 0;
    }

    MemoryFrame::MemoryFrame(
        core::memory::scratch_allocator& alloc,
        iceshard::Engine& engine,
        iceshard::IceshardExecutionInstance& execution_instance
    ) noexcept
        : iceshard::Frame{ }
        , _frame_allocator{ alloc }
        , _engine{ engine }
        , _execution_instance{ execution_instance }
        , _inputs_allocator{ _frame_allocator, detail::inputs_allocator_pool }
        , _message_allocator{ _frame_allocator, detail::message_allocator_pool }
        , _storage_allocator{ _frame_allocator, detail::storage_allocator_pool }
        , _data_allocator{ _frame_allocator, detail::data_allocator_pool }
        , _input_queue{ _inputs_allocator }
        , _input_events{ _inputs_allocator }
        , _input_actions{ _inputs_allocator }
        , _frame_messages{ _message_allocator }
        , _frame_storage{ _storage_allocator }
    {
        core::message::push(_frame_messages, FrameMessage{ detail::next_frame_index++, core::datetime::now().tick });
    }

    MemoryFrame::~MemoryFrame() noexcept
    {
        core::datetime::tick_type frame_end_tick = core::datetime::now().tick;
        core::datetime::tick_type frame_begin_tick;
        core::message::filter<FrameMessage>(_frame_messages, [&frame_begin_tick](FrameMessage const& msg) noexcept
            {
                frame_begin_tick = msg.tick;
            });

        auto const tick_count = frame_end_tick.value - frame_begin_tick.value;
        if (tick_count > 4001310)
        {
            fmt::print("Ticks on frame: {}\n", tick_count);
        }

        core::message::clear(_frame_messages);
        for (const auto& entry : _frame_storage)
        {
            entry.value.object_deleter(frame_allocator(), entry.value.object_instance);
        }
    }

    auto MemoryFrame::engine() noexcept -> Engine&
    {
        return _engine;
    }

    auto MemoryFrame::messages() const noexcept -> const core::MessageBuffer&
    {
        return _frame_messages;
    }

    auto MemoryFrame::input_queue() const noexcept -> iceshard::input::DeviceInputQueue const&
    {
        return _input_queue;
    }

    auto MemoryFrame::input_events() const noexcept -> core::pod::Array<iceshard::input::InputEvent> const&
    {
        return _input_events;
    }

    auto MemoryFrame::input_actions() const noexcept -> core::pod::Array<core::stringid_type> const&
    {
        return _input_actions;
    }

    auto MemoryFrame::find_frame_object(core::stringid_arg_type name) noexcept -> void*
    {
        uint64_t hash_value = static_cast<uint64_t>(name.hash_value);
        return core::pod::hash::get<frame_object_entry>(_frame_storage, hash_value, { nullptr, nullptr }).object_instance;
    }

    auto MemoryFrame::find_frame_object(core::stringid_arg_type name) const noexcept -> const void*
    {
        uint64_t hash_value = static_cast<uint64_t>(name.hash_value);
        return core::pod::hash::get<frame_object_entry>(_frame_storage, hash_value, { nullptr, nullptr }).object_instance;
    }

    void MemoryFrame::add_frame_object(core::stringid_arg_type name, void* frame_object, void(*deleter)(core::allocator&, void*)) noexcept
    {
        uint64_t hash_value = static_cast<uint64_t>(name.hash_value);

        IS_ASSERT(!core::pod::hash::has(_frame_storage, hash_value), "Frame object with name '{}' already exists!");
        core::pod::hash::set(_frame_storage, hash_value, { frame_object, deleter });
    }

    auto MemoryFrame::frame_allocator() noexcept -> core::allocator&
    {
        return _data_allocator;
    }

    auto MemoryFrame::engine_clock() const noexcept -> core::Clock const&
    {
        return _execution_instance.engine_clock();
    }

    auto MemoryFrame::elapsed_time() const noexcept -> float
    {
        return core::clock::elapsed(engine_clock());
    }

    void MemoryFrame::add_task(cppcoro::task<> task) noexcept
    {
        _execution_instance.add_task(std::move(task));
    }

} // namespace iceshard
