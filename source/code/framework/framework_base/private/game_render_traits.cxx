#include <ice/game_render_traits.hxx>
#include "traits/render/trait_render_gfx.hxx"
#include "traits/render/trait_render_clear.hxx"
#include "traits/render/trait_render_resource.hxx"
#include "traits/render/trait_render_postprocess.hxx"
#include "traits/render/trait_render_finish.hxx"
#include "traits/render/trait_render_sprites.hxx"
#include "traits/render/trait_render_tilemap.hxx"

namespace ice
{

    auto create_trait_render_gfx(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>
    {
        return ice::make_unique<ice::gfx::GfxTrait, ice::IceWorldTrait_RenderGfx>(alloc);
    }

    auto create_trait_render_clear(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>
    {
        return ice::make_unique<ice::gfx::GfxTrait, ice::IceWorldTrait_RenderClear>(alloc, stage_name);
    }

    auto create_trait_render_postprocess(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>
    {
        return ice::make_unique<ice::gfx::GfxTrait, ice::IceWorldTrait_RenderPostProcess>(alloc, stage_name);
    }

    auto create_trait_render_finish(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>
    {
        return ice::make_unique<ice::gfx::GfxTrait, ice::IceWorldTrait_RenderFinish>(alloc, stage_name);
    }

    auto create_trait_render_sprites(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>
    {
        return ice::make_unique<ice::gfx::GfxTrait, ice::IceWorldTrait_RenderSprites>(alloc, alloc, stage_name);
    }

    auto create_trait_render_tilemap(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>
    {
        return ice::make_unique<ice::gfx::GfxTrait, ice::IceWorldTrait_RenderTilemap>(alloc, alloc, stage_name);
    }

} // namespace ice
