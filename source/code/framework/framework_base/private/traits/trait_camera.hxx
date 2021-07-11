#pragma once
#include <ice/entity/entity.hxx>
#include <ice/game_camera.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    struct TraitCameraRenderData
    {
        ice::mat4x4 view;
        ice::mat4x4 projection;
        ice::mat4x4 clip;
    };

    struct TraitCameraData
    {
        ice::Entity entity;
        ice::render::Buffer uniform_buffer;
        ice::TraitCameraRenderData render_data;
    };

    class IceWorldTrait_RenderCamera : public ice::WorldTrait
    {
    public:
        IceWorldTrait_RenderCamera(ice::Allocator& alloc) noexcept;

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

    protected:
        auto task_update_cameras(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept -> ice::Task<>;

        auto task_update_camera_data(
            ice::render::RenderDevice& device,
            ice::TraitCameraData& render_data
        ) noexcept -> ice::Task<>;

    private:
        ice::pod::Hash<ice::TraitCameraData*> _camera_data;
    };

} // namespace ice
