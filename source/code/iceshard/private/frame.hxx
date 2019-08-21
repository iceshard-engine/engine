#include <iceshard/frame.hxx>

#include <core/allocator.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>

namespace iceshard
{


    //! \brief A single engien frame with a preallocated ring buffer allocator.
    class MemoryFrame : public iceshard::Frame
    {
    public:
        MemoryFrame(core::memory::scratch_allocator& alloc) noexcept;
        ~MemoryFrame() noexcept;

        //! \copydoc Frame::messages() noexcept
        auto messages() noexcept -> core::MessageBuffer& { return _frame_messages; }

        //! \copydoc Frame::messages() const noexcept
        auto messages() const noexcept -> const core::MessageBuffer& override;

        //! \copydoc Frame::frame_allocator() noexcept
        auto frame_allocator() noexcept -> core::allocator& override;

    private:
        core::memory::scratch_allocator& _frame_allocator;

        core::memory::scratch_allocator _message_allocator;
        core::memory::scratch_allocator _data_allocator;

        core::MessageBuffer _frame_messages;
    };


} // namespace iceshard
