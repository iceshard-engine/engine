#include <iceshard/frame.hxx>

#include <core/allocator.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>

namespace iceshard
{


    class MemoryFrame : public iceshard::Frame
    {
    public:
        MemoryFrame(core::memory::scratch_allocator& alloc) noexcept;
        ~MemoryFrame() noexcept;

        auto messages() noexcept -> core::MessageBuffer& { return _frame_messages; }

        auto messages() const noexcept -> const core::MessageBuffer& override;

        auto frame_allocator() noexcept -> core::allocator& override;

    private:
        core::memory::scratch_allocator& _frame_allocator;

        core::memory::scratch_allocator _message_allocator;
        core::memory::scratch_allocator _data_allocator;

        core::MessageBuffer _frame_messages;
    };


} // namespace iceshard
