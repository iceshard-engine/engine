#include <renderlib/render_target.h>
#include <renderlib/render_texture.h>

#include <kernel/logger/logger.h>

mooned::render::RenderTarget::RenderTarget(Type type)
    : _type{ type }
    , _handle{ 0 }
    , _internal{ 0 }
    , _color_attachment{ nullptr }
    , _depthstencil_attachment{ DepthStencilAttachment::Type::NONE, 0, 0 }
{
}

mooned::render::RenderTarget::~RenderTarget()
{
}

void mooned::render::RenderTarget::set_color_attachment(render::Texture* attachment)
{
    _color_attachment = attachment;
}

void mooned::render::RenderTarget::set_depth_stencil_attachment(DepthStencilAttachment attachment)
{
    _depthstencil_attachment = attachment;
}

bool mooned::render::RenderTarget::valid() const
{
    bool result = true;

    if (_type == Type::DEVICE)
    {
        result &= (_color_attachment == nullptr);
        result &= (_depthstencil_attachment.type == DepthStencilAttachment::Type::NONE);
    }
    else
    {
        result = false;
        if (_color_attachment)
        {
            assert(_color_attachment->valid());
            result = true;
        }

        if (_depthstencil_attachment.type != DepthStencilAttachment::Type::NONE)
        {
            bool partial = true;
            partial &= _depthstencil_attachment.width > 0;
            partial &= _depthstencil_attachment.height > 0;
            result |= partial;
        }

        if (_color_attachment && _depthstencil_attachment.type != DepthStencilAttachment::Type::NONE)
        {
            auto& details = _color_attachment->details();
            result &= (details.width == _depthstencil_attachment.width);
            result &= (details.height == _depthstencil_attachment.height);

            if (!result)
            {
                MLogError("Render target attachments have inconsistent sizes defined! Color [w:{}, h:{}] != DepthStencil [w:{}, h:{}]", details.width, details.height, _depthstencil_attachment.width, _depthstencil_attachment.height);
            }
        }
    }

    return result;
}