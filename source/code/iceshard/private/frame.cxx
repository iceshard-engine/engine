#include "frame.hxx"

#include <core/debug/assert.hxx>
#include <core/message/operations.hxx>
#include <core/datetime/datetime.hxx>
#include <core/pod/hash.hxx>

namespace iceshard
{
    namespace detail
    {
        static constexpr uint32_t MiB = 1024 * 1024;
        static constexpr uint32_t message_allocator_pool = 24 * MiB;
        static constexpr uint32_t storage_allocator_pool = 8 * MiB;
        static constexpr uint32_t data_allocator_pool = 196 * MiB;

        static uint32_t next_frame_index = 0;
    }

    MemoryFrame::MemoryFrame(core::memory::scratch_allocator& alloc) noexcept
        : iceshard::Frame{ }
        , _frame_allocator{ alloc }
        , _message_allocator{ _frame_allocator, detail::message_allocator_pool }
        , _storage_allocator{ _frame_allocator, detail::storage_allocator_pool }
        , _data_allocator{ _frame_allocator, detail::data_allocator_pool }
        , _frame_messages{ _message_allocator }
        , _frame_storage{ _storage_allocator }
    {
        core::message::push(_frame_messages, FrameMessage{ detail::next_frame_index++, core::datetime::now().tick });
    }

    MemoryFrame::~MemoryFrame() noexcept
    {
        core::message::clear(_frame_messages);
        for (const auto& entry : _frame_storage)
        {
            entry.value.object_deleter(frame_allocator(), entry.value.object_instance);
        }
    }

    auto MemoryFrame::messages() const noexcept -> const core::MessageBuffer&
    {
        return _frame_messages;
    }

    auto MemoryFrame::find_frame_object(core::cexpr::stringid_argument_type name) noexcept -> void*
    {
        uint64_t hash_value = static_cast<uint64_t>(name.hash_value);
        return core::pod::hash::get<frame_object_entry>(_frame_storage, hash_value, { nullptr, nullptr }).object_instance;
    }

    auto MemoryFrame::find_frame_object(core::cexpr::stringid_argument_type name) const noexcept -> const void*
    {
        uint64_t hash_value = static_cast<uint64_t>(name.hash_value);
        return core::pod::hash::get<frame_object_entry>(_frame_storage, hash_value, { nullptr, nullptr }).object_instance;
    }

    void MemoryFrame::add_frame_object(core::cexpr::stringid_argument_type name, void* frame_object, void(*deleter)(core::allocator&, void*)) noexcept
    {
        uint64_t hash_value = static_cast<uint64_t>(name.hash_value);

        IS_ASSERT(!core::pod::hash::has(_frame_storage, hash_value), "Frame object with name '{}' already exists!");
        core::pod::hash::set(_frame_storage, hash_value, { frame_object, deleter });
    }

    auto MemoryFrame::frame_allocator() noexcept -> core::allocator&
    {
        return _data_allocator;
    }

} // namespace iceshard