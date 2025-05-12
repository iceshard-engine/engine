/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_camera.hxx>

#include <ice/ecs/ecs_query.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_trait_descriptor.hxx>

namespace ice
{

    struct TraitCameraData
    {
        ice::StringID name;
        ice::CameraData camera_data;
        ice::render::Buffer render_data;
    };

    class IceWorldTrait_RenderCamera : public ice::Trait
    {
    public:
        static auto trait_descriptor() noexcept -> ice::TraitDescriptor const&;

        IceWorldTrait_RenderCamera(
            ice::Allocator& alloc,
            ice::TraitContext& context
        ) noexcept;

        auto activate(
            ice::WorldStateParams const& params
        ) noexcept -> ice::Task<> override;

        auto deactivate(
            ice::WorldStateParams const& params
        ) noexcept -> ice::Task<> override;

    protected:
        auto on_update(
            ice::EngineFrameUpdate const& params
        ) noexcept -> ice::Task<>;

        auto on_gfx_update(
            ice::render::RenderDevice& device,
            ice::DataStorage& data
        ) noexcept -> ice::Task<>;

        auto on_gfx_shutdown(
            ice::render::RenderDevice& device
        ) noexcept -> ice::Task<>;

    protected:
        auto task_update_cameras(
            ice::EngineFrame& frame
        ) noexcept -> ice::Task<>;

    private:
        ice::HashMap<ice::TraitCameraData> _render_data;

        ice::TaskCheckpoint _ready;
    };

} // namespace ice
