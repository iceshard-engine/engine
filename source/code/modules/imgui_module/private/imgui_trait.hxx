#pragma once
#include <ice/devui/devui_render_trait.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/asset_system.hxx>
#include <ice/clock.hxx>

namespace ice::devui
{

    class ImGuiTrait final : public ice::devui::DevUITrait, public ice::gfx::GfxStage
    {
    public:
        ImGuiTrait(ice::Allocator& alloc) noexcept;
        ~ImGuiTrait() noexcept override;

        auto gfx_stage_info() const noexcept -> ice::gfx::GfxStageInfo override;
        auto gfx_stage_slot() const noexcept -> ice::gfx::GfxStageSlot override;

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

        void record_commands(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

        bool start_frame() noexcept;
        void end_frame(
            ice::EngineFrame& frame
        ) noexcept;

    protected:
        auto task_create_render_objects(
            ice::EngineRunner& runner,
            ice::AssetSystem& asset_system,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_destroy_render_objects(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

        auto task_update_buffers(
            ice::EngineFrame& frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<>;

    private:
        bool _next_frame = false;
        bool _initialized = false;
        ice::vec2u _display_size;
        ice::Timer _imgui_timer;

        ice::render::ResourceSetLayout _resource_layout;
        ice::render::ResourceSet _resources;
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;

        ice::render::Sampler _sampler;
        ice::render::Image _font_texture;
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];

        ice::pod::Array<ice::render::Buffer> _index_buffers;
        ice::pod::Array<ice::render::Buffer> _vertex_buffers;
    };

} // namespace ice::devui
