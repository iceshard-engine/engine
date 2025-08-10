/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/asset_storage.hxx>
#include <ice/clock.hxx>

#include "imgui_system.hxx"
#include "imgui_gfx_stage.hxx"

namespace ice::devui
{

    class ImGuiSystem;

    struct ImGuiStats
    {
        ice::u32 draw_calls;
        ice::u32 draw_vertices;
        ice::u32 draw_indices;
        ice::Tns draw_processtime;
        ice::usize draw_datasize;
    };

    class ImGuiTrait final
        : public ice::Trait
        , public ice::TraitDevUI
        , public ice::InterfaceSelectorOf<ImGuiTrait, ice::TraitDevUI>
    {
    public:
        ImGuiTrait(ice::TraitContext& ctx, ice::Allocator& alloc, ImGuiSystem& system) noexcept;
        ~ImGuiTrait() noexcept override;

        auto activate(ice::WorldStateParams const& params) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& params) noexcept -> ice::Task<> override;

        auto update(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>;

        auto on_window_resized(ice::vec2i new_size) noexcept -> ice::Task<>;

    public:
        void build_content() noexcept override;

        auto trait_name() const noexcept -> ice::String override { return "ImGui.DevUI"; };

    public: // Gfx State Events
        auto gfx_start(
            ice::gfx::GfxStateChange const& params,
            ice::gfx::GfxContext& ctx,
            ice::AssetStorage& assets
        ) noexcept -> ice::Task<>;

        auto gfx_shutdown(ice::gfx::GfxStateChange const& params) noexcept -> ice::Task<>;
        auto gfx_update(ice::gfx::RenderFrameUpdate const& update) noexcept -> ice::Task<>;

    private:
        void build_internal_command_list(ice::Span<ImGuiGfxStage::DrawCommand> draw_cmds) noexcept;

    private:
        ice::ProxyAllocator _allocator;
        ice::devui::ImGuiSystem& _system;
        ice::devui::ImGuiStats _stats;
        ice::UniquePtr<ImGuiGfxStage> _imgui_gfx_stage;
        bool _resized;
    };

} // namespace ice::devui
