/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_camera.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/container/array.hxx>
#include <ice/ecs/ecs_query.hxx>

namespace ice
{

    using QueryCameras = ice::ecs::QueryDefinition<
        ice::ecs::EntityHandle,
        ice::Camera const&,
        ice::CameraOrtho const*,
        ice::CameraPerspective const*
    >;

    struct TraitCameraData
    {
        ice::StringID camera_name;
        ice::CameraData render_data;
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

    protected:
        auto task_update_cameras(
            ice::EngineFrame& frame
        ) noexcept -> ice::Task<>;

    private:
        ice::ecs::Query<ice::QueryCameras> _query_cameras;
        ice::Array<ice::TraitCameraData> _cameras;
        ice::TaskCheckpoint _ready;
    };

} // namespace ice
