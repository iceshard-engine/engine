#include "frame.hxx"

#include <core/debug/assert.hxx>
#include <core/message/operations.hxx>
#include <core/datetime/datetime.hxx>

namespace iceshard
{
    namespace detail
    {
        static uint32_t next_frame_index = 0;
    }

    CoroutineFrame::CoroutineFrame(core::memory::scratch_allocator& alloc) noexcept
        : iceshard::Frame{ }
        , _frame_allocator{ alloc }
        , _frame_messages{ _frame_allocator }
    {
        [[maybe_unused]]
        auto allocator_was_empty = _frame_allocator.reset();
        IS_ASSERT(allocator_was_empty == true, "The previous frame did not clear all objects!");

        core::message::push(_frame_messages, FrameMessage{ detail::next_frame_index++, core::datetime::now().tick });
    }

    CoroutineFrame::~CoroutineFrame() noexcept
    {
    }

    auto CoroutineFrame::messages() const noexcept -> const core::MessageBuffer&
    {
        return _frame_messages;
    }

    auto CoroutineFrame::frame_allocator() noexcept -> core::allocator&
    {
        return _frame_allocator;
    }

} // namespace iceshard