#pragma once
#include <ice/render/render_declarations.hxx>

namespace ice::gfx
{

    struct GfxResource
    {
        enum class Type : ice::u32
        {
            Invalid = 0x0,
            Framebuffer,
            Renderpass,
            ResourceSetLayout,
            ResourceSet,
            PipelineLayout,
            Pipeline,
            Shader,
            Image,
            Sampler,
            Buffer
        };

        Type type;

        union
        {
            ice::render::Framebuffer framebuffer;
            ice::render::Renderpass renderpass;
            ice::render::ResourceSetLayout resourceset_layout;
            ice::render::ResourceSet resourceset;
            ice::render::PipelineLayout pipeline_layout;
            ice::render::Pipeline pipeline;
            ice::render::Shader shader;
            ice::render::Image image;
            ice::render::Sampler sampler;
            ice::render::Buffer buffer;
        } value;
    };

    static_assert(sizeof(GfxResource) <= 16, "GfxResource object is bigger than planned!");

} // namespace ice::gfx
