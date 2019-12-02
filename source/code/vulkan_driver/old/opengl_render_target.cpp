#include <renderlib/render_target.h>
#include <renderlib/render_texture.h>
#include "opengl_enums.h"

#include <gl/glew.h>
#include <gl/GL.h>

#include <cassert>

namespace mooned::render
{
namespace opengl
{

GLenum depth_stencil_attachment(RenderTarget::DepthStencilAttachment::Type type)
{
    GLenum result = GL_INVALID_ENUM;

    switch (type)
    {
    case mooned::render::RenderTarget::DepthStencilAttachment::Type::DEPTH24_STENCIL8:
        result = GL_DEPTH24_STENCIL8;
        break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

} // namespace opengl

RenderTarget::Handle RenderTarget::get_handle()
{
    assert(valid());

    if (_handle == 0)
    {
        assert(_internal == 0);

        glGenFramebuffers(1, std::addressof(_handle._value));
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<uint32_t>(_handle));

        if (_color_attachment)
        {
            assert(_color_attachment->valid());
            //assert(_color_attachment->type() == TextureDetails::Type::TEXTURE_2D);
            assert(_color_attachment->details().mipmap_level == 0);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, togl(_color_attachment->type()), static_cast<uint32_t>(_color_attachment->get_handle()), 0);
        }

        if (_depthstencil_attachment.type != DepthStencilAttachment::Type::NONE)
        {
            glGenRenderbuffers(1, std::addressof(_internal._value));
            glBindRenderbuffer(GL_RENDERBUFFER, static_cast<uint32_t>(_internal));

            if (_depthstencil_attachment.sample_number > 0)
            {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, _depthstencil_attachment.sample_number, opengl::depth_stencil_attachment(_depthstencil_attachment.type), _depthstencil_attachment.width, _depthstencil_attachment.height);
            }
            else
            {
                glRenderbufferStorage(GL_RENDERBUFFER, opengl::depth_stencil_attachment(_depthstencil_attachment.type), _depthstencil_attachment.width, _depthstencil_attachment.height);
            }

            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, static_cast<uint32_t>(_internal));
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    return _handle;
}

void RenderTarget::release_handle()
{
    if (_handle != 0)
    {
        if (_internal != 0)
        {
            glDeleteRenderbuffers(1, std::addressof(_internal._value));
            _internal = 0;
        }

        glDeleteFramebuffers(1, std::addressof(_internal._value));
        _handle = 0;
    }
}

} // namespace mooned::render
