/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_trait.hxx>
#include <ice/game_camera.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    struct TraitCameraRenderData
    {
        ice::mat4x4 view;
        ice::mat4x4 projection;
    };

    struct TraitCameraData
    {
        ice::StringID camera_name;
        ice::TraitCameraRenderData render_data;
    };

    class IceWorldTrait_RenderCamera : public ice::gfx::GfxTrait
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

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

    protected:
        auto task_update_cameras(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept -> ice::Task<>;

    private:
        ice::Array<ice::render::Buffer> _camera_buffers;
    };

} // namespace ice
