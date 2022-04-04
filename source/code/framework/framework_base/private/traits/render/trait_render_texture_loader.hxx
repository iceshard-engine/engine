#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/gfx/gfx_device.hxx>

#include <ice/pod/hash.hxx>

namespace ice
{

    class AssetRequest;

    class IceWorldTrait_RenderTextureLoader : public ice::gfx::GfxTrait
    {
    public:
        IceWorldTrait_RenderTextureLoader(
            ice::Allocator& alloc
        ) noexcept;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

    private:
        auto load_image(
            ice::AssetRequest* request,
            ice::EngineRunner& runner
        ) noexcept -> ice::Task<>;

    private:
        ice::pod::Hash<ice::render::Image> _loaded_images;
    };

} // namespace ice
