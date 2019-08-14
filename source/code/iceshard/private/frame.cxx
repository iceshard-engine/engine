#include "frame.hxx"

#include <core/debug/assert.hxx>
#include <core/message/operations.hxx>
#include <core/datetime/datetime.hxx>

namespace iceshard
{
    namespace detail
    {
        static constexpr uint32_t MiB = 1024 * 1024;
        static constexpr uint32_t message_allocator_pool = 32 * MiB;
        static constexpr uint32_t data_allocator_pool = 200 * MiB;

        static uint32_t next_frame_index = 0;
    }

    CoroutineFrame::CoroutineFrame(core::memory::scratch_allocator& alloc) noexcept
        : iceshard::Frame{ }
        , _frame_allocator{ alloc }
        , _message_allocator{ _frame_allocator, detail::message_allocator_pool }
        , _data_allocator{ _frame_allocator, detail::data_allocator_pool }
        , _frame_messages{ _message_allocator }
    {
        core::message::push(_frame_messages, FrameMessage{ detail::next_frame_index++, core::datetime::now().tick });
    }

    CoroutineFrame::~CoroutineFrame() noexcept
    {
        core::message::clear(_frame_messages);
    }

    auto CoroutineFrame::messages() const noexcept -> const core::MessageBuffer&
    {
        return _frame_messages;
    }

    auto CoroutineFrame::frame_allocator() noexcept -> core::allocator&
    {
        return _data_allocator;
    }

} // namespace iceshard