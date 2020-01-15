#pragma once
#include <core/allocator.hxx>
#include <core/pod/collections.hxx>
#include <core/message/buffer.hxx>
#include <core/datetime/types.hxx>

namespace iceshard
{


    //! \brief A simple message pushed at the beginning on each new frame.
    struct FrameMessage
    {
        static constexpr auto message_type = "FrameMessage"_sid;

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

        ////! \brief Returns a flat map for storing data pointers.
        //virtual auto storage() noexcept -> core::pod::Hash<void*>& = 0;

        ////! \brief Returns a flat map for storing data pointers.
        //virtual auto storage() const noexcept -> const core::pod::Hash<void*>& = 0;

        virtual auto find_frame_object(core::stringid_arg_type name) noexcept -> void* = 0;

        virtual auto find_frame_object(core::stringid_arg_type name) const noexcept -> const void* = 0;

        virtual void add_frame_object(core::stringid_arg_type name, void* frame_object, void(*deleter)(core::allocator&, void*)) noexcept = 0;

        template<typename T>
        auto get_frame_object(core::stringid_arg_type name) noexcept -> T*;

        template<typename T>
        auto get_frame_object(core::stringid_arg_type name) const noexcept -> const T*;

        template<typename T, typename... Args>
        auto new_frame_object(core::stringid_arg_type name, Args&&... args) noexcept -> T*;

        //! \brief Returns this frame allocator object.
        //!
        //! \remarks This allocator will be forcibly released after two frames.
        //!     so any data which needs to persist between frames needs to be copied each frame!
        //! \remarks The returned allocator is ensured to be optimized performance wise, but has a upper memory limit.
        virtual auto frame_allocator() noexcept -> core::allocator& = 0;
    };


    template<typename T>
    auto Frame::get_frame_object(core::stringid_arg_type name) noexcept -> T*
    {
        return reinterpret_cast<T*>(find_frame_object(name));
    }

    template<typename T>
    auto Frame::get_frame_object(core::stringid_arg_type name) const noexcept -> const T*
    {
        return reinterpret_cast<const T*>(find_frame_object(name));
    }

    template<typename T, typename... Args>
    auto Frame::new_frame_object(core::stringid_arg_type name, Args&&... args) noexcept -> T*
    {
        void(*object_instance_deleter)(core::allocator&, void*) = [](core::allocator& alloc, void* object) noexcept
        {
            alloc.destroy(reinterpret_cast<T*>(object));
        };

        auto* object_instance = frame_allocator().make<T>(std::forward<Args>(args)...);
        add_frame_object(name, object_instance, object_instance_deleter);
        return object_instance;
    }


} // namespace iceshard
