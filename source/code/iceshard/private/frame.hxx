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

        //auto storage() noexcept -> core::pod::Hash<void*>& override { return _frame_storage; }

        //auto storage() const noexcept -> const core::pod::Hash<void*>& override { return _frame_storage; }

        auto find_frame_object(core::cexpr::stringid_argument_type name) noexcept -> void* override;

        auto find_frame_object(core::cexpr::stringid_argument_type name) const noexcept -> const void* override;

        void add_frame_object(core::cexpr::stringid_argument_type name, void* frame_object, void(*deleter)(core::allocator&, void*)) noexcept override;

        //! \copydoc Frame::frame_allocator() noexcept
        auto frame_allocator() noexcept -> core::allocator& override;

    private:
        core::memory::scratch_allocator& _frame_allocator;

        core::memory::scratch_allocator _message_allocator;
        core::memory::scratch_allocator _storage_allocator;
        core::memory::scratch_allocator _data_allocator;

        core::MessageBuffer _frame_messages;

        struct frame_object_entry
        {
            void* object_instance;
            void(*object_deleter)(core::allocator&, void*);
        };
        core::pod::Hash<frame_object_entry> _frame_storage;
    };


} // namespace iceshard
