/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_declarations.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/ui_element_draw.hxx>
#include <ice/shard.hxx>

#if 0
namespace ice
{

    static constexpr ice::Shard Shard_RenderUIData = "action/render/ui`ice::RenderUIRequest const*"_shard;

    enum class RenderUIRequestType
    {
        CreateOrUpdate,
        UpdateAndShow,
        UpdateAndHide,
        Disable,
        Enable,
        Destroy,
    };

    struct RenderUIRequest
    {
        ice::u64 id;
        ice::vec2f position;
        ice::ui::DrawData const* draw_data;
        ice::RenderUIRequestType type;
    };

    struct RenderUIData
    {
        ice::u64 id;
        struct Uniform
        {
            ice::vec2f position;
            ice::vec2f scale;
        };

        Uniform uniform;

        ice::render::Buffer buffer_uniform;
        ice::render::Buffer buffer_vertices;
        ice::render::Buffer buffer_colors;

        ice::render::ResourceSet resourceset_uniform;

        ice::ui::DrawData const* draw_data;

        bool is_dirty;
        bool is_enabled;
    };

    struct RenderUICommand
    {
        ice::RenderUIData const* render_data;
        ice::u32 vertice_count;
    };

    class IceWorldTrait_RenderUI final : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        IceWorldTrait_RenderUI(
            ice::Allocator& alloc
        ) noexcept;

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxContext& gfx_ctx
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxContext& gfx_ctx
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxContext& gfx_ctx
        ) noexcept override;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

    private:
        ice::Data _shader_data[2];
        ice::render::Shader _shaders[2];
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;

        ice::render::ResourceSetLayout _resource_set_layout[1];

        ice::vec2f _display_size;
        ice::HashMap<RenderUIData*> _render_data;
    };

    void register_trait_render_ui(ice::WorldTraitArchive& archive) noexcept;

} // namespace ice

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::RenderUIRequest const*> = ice::shard_payloadid("ice::RenderUIRequest const*");

#endif
