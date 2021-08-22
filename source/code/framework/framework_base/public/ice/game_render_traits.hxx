#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_trait.hxx>

namespace ice
{

    static constexpr ice::Shard Shard_SetDefaultCamera = "action/camera/set-default"_shard;
    static constexpr ice::Shard Shard_DebugDrawCommand = "action/debug-render/draw_command"_shard;

    auto create_trait_camera(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

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

    auto create_trait_render_tilemap(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

    auto create_trait_render_debug(
        ice::Allocator& alloc,
        ice::StringID_Arg stage_name
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>;

    struct DebugDrawCommand
    {
        ice::u32 vertex_count;
        ice::vec3f const* vertex_list;
        ice::vec1u const* vertex_color_list;
    };

    struct DebugDrawCommandList
    {
        ice::u32 list_size;
        ice::DebugDrawCommand const* list;
    };

} // namespace ice

template<>
constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::StringID_Hash> = ice::payload_id("ice::StringID_Hash");

template<>
constexpr ice::PayloadID ice::detail::Constant_ShardPayloadID<ice::DebugDrawCommandList const*> = ice::payload_id("ice::DebugDrawCommandList const*");
