#pragma once
#include <render_system/render_command_buffer.hxx>
#include <asset_system/assets/asset_shader.hxx>
#include <iceshard/renderer/render_system.hxx>

namespace render
{

    //! \brief Render context interface.
    class RenderContext;

    //! \brief A render system interface.
    class RenderSystem : public iceshard::renderer::RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual void prepare() noexcept { }

        virtual auto create_buffer(render::api::BufferType type, uint32_t size) noexcept -> render::api::Buffer = 0;

        virtual auto create_vertex_buffer(uint32_t size) noexcept -> render::api::VertexBuffer = 0;

        virtual auto create_uniform_buffer(uint32_t size) noexcept -> render::api::UniformBuffer = 0;

        virtual auto load_texture(asset::AssetData texture_data) noexcept -> render::api::Texture = 0;

        virtual void initialize_render_interface(render::api::RenderInterface** render_interface) noexcept = 0;

        virtual void swap() noexcept = 0;
    };

} // namespace render
