/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/asset_storage.hxx>
#include <ice/clock.hxx>

#include "imgui_gfx_stage.hxx"

namespace ice::devui
{

    class ImGuiSystem;

    class ImGuiTrait final : public ice::Trait
    {
    public:
        ImGuiTrait(ice::Allocator& alloc, ImGuiSystem& system) noexcept;
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

    private:
        void build_internal_command_list(ice::Span<ImGuiGfxStage::DrawCommand> draw_cmds) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::UniquePtr<ImGuiGfxStage> _imgui_gfx_stage;
        bool _resized;
    };

} // namespace ice::devui
