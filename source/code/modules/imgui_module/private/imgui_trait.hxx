#pragma once
#include <ice/devui/devui_render_trait.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/asset_storage.hxx>
#include <ice/clock.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    class ImGuiTrait final : public ice::devui::DevUITrait, public ice::gfx::GfxContextStage
    {
    public:
        ImGuiTrait(ice::Allocator& alloc) noexcept;
        ~ImGuiTrait() noexcept override;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

        bool start_frame() noexcept;
        void end_frame(
            ice::EngineFrame& frame
        ) noexcept;

        auto imgui_context() const noexcept -> ImGuiContext*;

    protected:
        void build_internal_command_list(ice::EngineFrame& frame) noexcept;

    private:
        bool _initialized = false;
        bool _next_frame = false;

        ImGuiContext* _imgui_context = nullptr;

        ice::vec2u _display_size;
        ice::Timer _imgui_timer;

        ice::render::ResourceSetLayout _resource_layout;
        ice::render::ResourceSet _resources[20];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;

        ice::render::Sampler _sampler;
        ice::render::Image _font_texture;
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];
        ice::Data _shader_data[2];

        ice::Array<ice::render::Buffer> _index_buffers;
        ice::Array<ice::render::Buffer> _vertex_buffers;
    };

} // namespace ice::devui
