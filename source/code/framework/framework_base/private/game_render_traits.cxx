#include <ice/game_render_traits.hxx>
#include "traits/render/trait_render_gfx.hxx"
#include "traits/render/trait_render_clear.hxx"

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

} // namespace ice
