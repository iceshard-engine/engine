#pragma once
#include <ice/engine.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/unique_ptr.hxx>

namespace ice::trait
{

    class Terrain final : public ice::WorldTrait, public ice::gfx::GfxStage
    {
    public:
        Terrain(
            ice::Allocator& alloc,
            ice::Engine& engine
        ) noexcept;
        ~Terrain() noexcept override = default;

        void on_activate(
            ice::Engine&,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine&,
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
        ) noexcept override;

        struct RenderCache;

    private:
        ice::Engine& _engine;
        ice::AssetSystem& _asset_system;
        ice::UniquePtr<RenderCache> _render_cache;

        bool _debug_pl = false;
    };

} // namespace ice::trait
