/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui/devui_render_trait.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/asset_storage.hxx>
#include <ice/clock.hxx>

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
            ice::render::Renderpass renderpass
        ) noexcept -> ice::Task<> override;

        auto cleanup(
            ice::gfx::GfxContext& gfx
        ) noexcept -> ice::Task<> override;

        void update(
            ice::gfx::GfxContext& device
        ) noexcept override;

        void draw(
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& render_api
        ) const noexcept override;

    private:
        ice::AssetStorage& _assets;

        ice::render::ResourceSetLayout _resource_layout;
        ice::render::ResourceSet _resources[20];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;

        ice::render::Sampler _sampler;
        ice::render::Image _font_texture;
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];

        ice::u16* _index_buffer_host;
        ice::Array<ice::render::Buffer> _index_buffers;
        ice::Array<ice::render::Buffer> _vertex_buffers;
        ice::render::Buffer _uniform_buffer;
    };

    class ImGuiTrait final : public ice::devui::DevUITrait
    {
    public:
        ImGuiTrait(ice::Allocator& alloc) noexcept;
        ~ImGuiTrait() noexcept override;

        void gather_tasks(ice::TraitTaskRegistry& task_launcher) noexcept override;

        auto activate(ice::WorldStateParams const& params) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& params) noexcept -> ice::Task<> override;

        auto update(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>;

        auto on_window_resized(ice::vec2i new_size) noexcept -> ice::Task<>;

    public: // Gfx State Events
        auto gfx_start(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>;
        auto gfx_shutdown(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>;
        auto gfx_update(ice::gfx::GfxFrameUpdate const& update) noexcept -> ice::Task<>;

        bool start_frame() noexcept;
        void end_frame(ice::EngineFrame& frame) noexcept;

        auto imgui_context() const noexcept -> ImGuiContext*;

    protected:
        void build_internal_command_list(ice::EngineFrame& frame) noexcept;

    private:
        ice::Allocator& _allocator;
        bool _initialized = false;
        bool _font_texture_loaded = false;
        bool _next_frame = false;

        ImGuiContext* _imgui_context = nullptr;
        ice::UniquePtr<ImGuiGfxStage> _imgui_gfx_stage;

        ice::vec2u _display_size;
        ice::Timer _imgui_timer;
    };

} // namespace ice::devui
