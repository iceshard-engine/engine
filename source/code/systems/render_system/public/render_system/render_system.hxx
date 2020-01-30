#pragma once
#include <render_system/render_command_buffer.hxx>
#include <render_system/render_vertex_descriptor.hxx>
#include <render_system/render_pipeline.hxx>
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

        virtual auto current_framebuffer() noexcept -> render::api::Framebuffer = 0;

        virtual auto create_buffer(render::api::BufferType type, uint32_t size) noexcept -> render::api::Buffer = 0;

        virtual auto create_vertex_buffer(uint32_t size) noexcept -> render::api::VertexBuffer = 0;

        virtual auto create_uniform_buffer(uint32_t size) noexcept -> render::api::UniformBuffer = 0;

        virtual auto load_texture(asset::AssetData texture_data) noexcept -> render::api::Texture = 0;

        virtual void load_shader(asset::AssetData shader_data) noexcept = 0;

        template<uint32_t Size>
        void add_named_vertex_descriptor_set(VertexDescriptorSet<Size> const& binding_set) noexcept;

        virtual void initialize_render_interface(render::api::RenderInterface** render_interface) noexcept = 0;

        virtual void swap() noexcept = 0;

    private:

        virtual void add_named_vertex_descriptor_set(
            core::stringid_arg_type name,
            VertexBinding const& binding,
            VertexDescriptor const* descriptors,
            uint32_t descriptor_count
        ) noexcept = 0;
    };

    template<uint32_t Size>
    inline void RenderSystem::add_named_vertex_descriptor_set(
        VertexDescriptorSet<Size> const& binding_set) noexcept
    {
        add_named_vertex_descriptor_set(binding_set.name, binding_set.binding, binding_set.descriptors, Size);
    }

} // namespace render
