#pragma once
#include <core/allocator.hxx>
#include <core/message/buffer.hxx>
#include <core/datetime/types.hxx>

namespace iceshard
{


    //! \brief A simple message pushed at the beginning on each new frame.
    struct FrameMessage
    {
        static constexpr core::cexpr::stringid_type message_type = core::cexpr::stringid_cexpr("FrameMessage");

        //! \brief The current frame index.
        uint32_t index;

        //! \brief The current frame tick.
        core::datetime::tick_type tick;
    };


    //! \brief Describes a single frame in the engine lifetime.
    class Frame
    {
    public:
        Frame() noexcept = default;
        virtual ~Frame() noexcept = default;

        // Moving not allowed
        Frame(Frame&&) noexcept = delete;
        auto operator=(Frame&&) noexcept -> Frame& = delete;

        // Copying not allowed
        Frame(const Frame&) noexcept = delete;
        auto operator=(const Frame&) noexcept -> Frame& = delete;

        //! \brief Returns this frame message buffer.
        virtual auto messages() const noexcept -> const core::MessageBuffer& = 0;

        //! \brief Returns this frame allocator object.
        //!
        //! \remarks This allocator will be forcibly released after two frames.
        //!     so any data which needs to persist between frames needs to be copied each frame!
        //! \remakrs The returned allocator is ensured to be optimized performance wise, but has a upper memory limit.
        virtual auto frame_allocator() noexcept -> core::allocator& = 0;
    };


} // namespace iceshard
