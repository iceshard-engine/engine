#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>

namespace ice
{

    class GameWorldTrait_Render : public ice::WorldTrait
    {
    public:
        virtual auto gfx_stage_name() const noexcept -> ice::StringID = 0;
        virtual auto gfx_stage() const noexcept -> ice::gfx::GfxStage const* = 0;
    };

    auto create_trait_render_gfx(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>;

    auto create_trait_render_clear(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::GameWorldTrait_Render>;

} // namespace ice
