#include <ice/gfx/gfx_pass.hxx>
#include <ice/render/render_pass.hxx>

#include "gfx_dynamic_pass.hxx"

namespace ice::gfx
{

    struct IceGfxSubPass
    {

    };

    class IceGfxStaticPass : public GfxPass
    {
    public:
        IceGfxStaticPass() noexcept;
        ~IceGfxStaticPass() noexcept override;
    };

    auto create_static_pass(
        ice::Allocator& alloc,
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxPassInfo const& pass_description
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxPass>
    {
        return ice::make_unique_null<ice::gfx::GfxPass>();
    }

    auto create_dynamic_pass(
        ice::Allocator& allocator
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxDynamicPass>
    {
        return ice::make_unique<ice::gfx::GfxDynamicPass, ice::gfx::IceGfxDynamicPass>(allocator, allocator);
    }

} // namespace ice::gfx
