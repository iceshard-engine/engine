#pragma once
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

        virtual auto create_buffer(iceshard::renderer::api::BufferType type, uint32_t size) noexcept -> iceshard::renderer::api::Buffer = 0;

        virtual auto load_texture(asset::AssetData texture_data) noexcept -> iceshard::renderer::api::Texture = 0;

        virtual void initialize_render_interface(iceshard::renderer::api::RenderInterface** render_interface) noexcept = 0;
    };

} // namespace render
