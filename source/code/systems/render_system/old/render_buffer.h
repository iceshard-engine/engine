#pragma once
#include <core/base.hxx>

namespace mooned::render
{


    //! \brief Enum of buffer targets available for use.
    enum class BufferTarget
    {
        ArrayBuffer,
        ElementArrayBuffer,
        UniformBuffer,
    };


    //! \brief A render buffer handle used in command buffers.
    class RenderBuffer
    {
    public:
        RenderBuffer(BufferTarget target) noexcept;
        ~RenderBuffer() noexcept;

    };


    //! \brief A single render buffer object.
    class RenderBuffer
    {
    public:
        using Handle = mooned::strong_numeric_typedef<RenderBuffer, uint32_t>;

        RenderBuffer();
        ~RenderBuffer() = default;

        //! Returns true if the render buffer is valid.
        bool valid() const;

        //! Returns the handle, or creates one if it does not exist yet.
        Handle get_handle();

        //! Releases the underlying handle, or does nothing if it does not exist.
        void release_handle();

    private:
        Handle _handle;
    };

}
