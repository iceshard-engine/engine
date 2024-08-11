#pragma once
#include <ice/gfx/gfx_stage.hxx>
#include <ice/render/render_pipeline.hxx>
#include <ice/engine_frame.hxx>
#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    class ImGuiGfxStage : public ice::gfx::GfxStage
    {
    public:
        ImGuiGfxStage(
            ice::Allocator& alloc,
            ice::AssetStorage& assets
        ) noexcept;

    public: // Implementation of: ice::gfx::GfxStage
        auto initialize(
            ice::gfx::GfxContext& gfx,
            ice::gfx::GfxFrameStages& stages,
            ice::render::Renderpass renderpass,
            ice::u32 subpass
        ) noexcept -> ice::Task<> override;

        auto cleanup(
            ice::gfx::GfxContext& gfx
        ) noexcept -> ice::Task<> override;

        void update(
            ice::EngineFrame const& frame,
            ice::gfx::GfxContext& device
        ) noexcept override;

        void draw(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept override;

        struct DrawCommand
        {
            ice::u32 resource_set_idx;
            ice::u32 index_count;
            ice::u32 index_offset;
            ice::u32 vertex_offset;
            ImVec4 clip_rect;
        };

        ice::Array<DrawCommand> draw_commands;

    private:
        ice::AssetStorage& _assets;

        ice::render::ResourceSetLayout _resource_layout[2];
        ice::render::ResourceSet _resources[20];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;

        ice::render::Sampler _sampler;
        ice::render::Image _font_texture;
        ice::render::PipelineProgramInfo _shaders[2];

        ice::u16* _index_buffer_host;
        ice::Array<ice::render::Buffer> _index_buffers;
        ice::Array<ice::render::Buffer> _vertex_buffers;
        ice::render::Buffer _uniform_buffer;
    };

} // namespace ice::devui
