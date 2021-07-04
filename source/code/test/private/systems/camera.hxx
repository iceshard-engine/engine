#pragma once
#include <ice/allocator.hxx>
#include <ice/engine.hxx>
#include <ice/world/world_trait.hxx>

namespace ice::trait
{

    class CameraManager final : public ice::WorldTrait, public ice::gfx::GfxStage
    {
    public:
        CameraManager(
            ice::Allocator& alloc,
            ice::Engine& engine
        ) noexcept;
        ~CameraManager() noexcept override;


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
            ice::render::CommandBuffer cmds,
            ice::render::RenderCommands& api
        ) const noexcept override;

    protected:
        ice::Allocator& _allocator;
        ice::Engine& _engine;

        struct RenderObjects;
        RenderObjects* const _render_objects;
    };

} // namespace ice::trait
