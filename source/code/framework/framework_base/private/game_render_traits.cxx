#include <ice/game_render_traits.hxx>
#include "traits/render/trait_render_gfx.hxx"
#include "traits/render/trait_render_clear.hxx"
#include "traits/render/trait_render_resource.hxx"
#include "traits/render/trait_render_postprocess.hxx"
#include "traits/render/trait_render_finish.hxx"
#include "traits/render/trait_render_sprites.hxx"

namespace ice
{

    auto create_trait_render_gfx(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>
    {
        return ice::make_unique<ice::GameWorldTrait_Render, ice::IceWorldTrait_RenderGfx>(alloc);
    }

    auto create_trait_render_clear(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>
    {
        return ice::make_unique<ice::GameWorldTrait_Render, ice::IceWorldTrait_RenderClear>(alloc);
    }

    auto create_trait_render_resource(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>
    {
        return ice::make_unique_null<ice::GameWorldTrait_Render>();
        //return ice::make_unique<ice::GameWorldTrait_Render, ice::IceWorldTrait_RenderResource>(alloc, alloc);
    }

    auto create_trait_render_postprocess(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>
    {
        return ice::make_unique<ice::GameWorldTrait_Render, ice::IceWorldTrait_RenderPostProcess>(alloc);
    }

    auto create_trait_render_finish(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>
    {
        return ice::make_unique<ice::GameWorldTrait_Render, ice::IceWorldTrait_RenderFinish>(alloc);
    }

    auto create_trait_render_sprites(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_RenderDraw>
    {
        return ice::make_unique<ice::GameWorldTrait_RenderDraw, ice::IceWorldTrait_RenderSprites>(alloc, alloc);
    }

} // namespace ice
