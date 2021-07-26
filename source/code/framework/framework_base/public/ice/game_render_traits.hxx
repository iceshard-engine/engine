#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_trait.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_SetDefaultCamera = "action/camera/set-default"_shard;

    auto create_trait_render_gfx(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

    auto create_trait_render_clear(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

    auto create_trait_render_postprocess(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

    auto create_trait_render_finish(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

    auto create_trait_render_sprites(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

} // namespace ice

template<>
constexpr ice::u32 ice::detail::Constant_ShardPayloadID<ice::StringID_Hash> = ice::hash32("ice::StringID_Hash");
