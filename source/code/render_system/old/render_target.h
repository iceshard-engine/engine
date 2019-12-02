#pragma once
#include <kernel/types.h>
#include <kernel/utils/strong_typedef.h>

namespace mooned::render
{

class Texture;

class RenderTarget
{
public:
    using Handle = mooned::strong_numeric_typedef<RenderTarget, uint32_t>;

    //! The type of a render tagret
    enum class Type
    {
        DEVICE, // Type of the default render target
        TEXTURE,
    };

    //! Describes available dept-stencil attachment's
    struct DepthStencilAttachment
    {
        enum class Type
        {
            NONE,
            DEPTH24_STENCIL8,
        } type;

        int width;
        int height;
        int sample_number{ 0 };
    };

    //! Creates a render target of the given type.
    RenderTarget(Type type);
    ~RenderTarget();

    //! Set the color attachment
    //! \note Only a texture can be set as a color attachment
    //! \note This object requires the texture to say valid during it's whole life.
    void set_color_attachment(render::Texture* attachment);

    //! Set the depth-stencil attachment values
    //! \note The attachment is created internally from the values provided.
    void set_depth_stencil_attachment(DepthStencilAttachment attachment);

    //! Returns true if the current definition is valid.
    bool valid() const;

    //! Returns a handle to be used in a render command buffer.
    //! \note If a handle does not exist, it will create one.
    Handle get_handle();

    //! Releases the underlying handle, which makes it unusable in render command buffers.
    //! \note If the handle does not exist, nothing happens.
    void release_handle();

private:
    const Type _type;

    Handle _handle;

    //! Additional internal handle
    Handle _internal;

    //! Attachments
    render::Texture* _color_attachment;
    DepthStencilAttachment _depthstencil_attachment;
};

}
