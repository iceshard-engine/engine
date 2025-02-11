/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_camera.hxx"
#include <ice/game_entity.hxx>
#include <ice/game_camera.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/world/world_trait_descriptor.hxx>

#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>

#include <ice/gfx/gfx_context.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>

#include <ice/math/lookat.hxx>
#include <ice/math/projection.hxx>

#include <ice/assert.hxx>

namespace ice
{
    auto IceWorldTrait_RenderCamera::trait_descriptor() noexcept -> ice::TraitDescriptor const&
    {
        static ice::TraitDescriptor const descriptor{
            .name = ice::TraitID_CameraManager,
            .fn_factory = ice::detail::default_trait_factory<IceWorldTrait_RenderCamera>
        };
        return descriptor;
    }

    IceWorldTrait_RenderCamera::IceWorldTrait_RenderCamera(
        ice::Allocator& alloc,
        ice::TraitContext& context
    ) noexcept
        : ice::Trait{ context }
        , _query_cameras{ alloc }
        , _cameras{ alloc }
    {
        context.bind<&IceWorldTrait_RenderCamera::on_update>();
        context.register_checkpoint("camera-data"_sid, _ready);
    }

    auto IceWorldTrait_RenderCamera::activate(
        ice::WorldStateParams const& params
    ) noexcept -> ice::Task<>
    {
        entity_queries().initialize_query(_query_cameras);
        co_return;
    }

    auto IceWorldTrait_RenderCamera::deactivate(
        ice::WorldStateParams const& params
    ) noexcept -> ice::Task<>
    {
        co_return;
    }

    auto IceWorldTrait_RenderCamera::on_update(
        ice::EngineFrameUpdate const& params
    ) noexcept -> ice::Task<>
    {
        //ice::u32 const camera_count = ice::ecs::query::entity_count(_query_cameras);
        ice::array::resize(_cameras, ice::ecs::query::entity_count(_query_cameras));

        //ice::u32 cam_idx = 0;
        for (auto[entity, camera, ortho, persp] : ice::ecs::query::for_each_entity(_query_cameras))
        {
            if (camera->name == ice::StringID_Invalid)
            {
                continue;
            }

            ICE_ASSERT(
                (ortho == nullptr && persp == nullptr) == false,
                "Camera is not properly set-up, missing orthographics or perspective parameters."
            );
            ICE_ASSERT(
                (ortho != nullptr && persp != nullptr) == false,
                "Camera is not properly set-up, defining both orthographics and perspective parameters."
            );

            static ice::mat4x4 const clip = {
                .v = {
                    { 1.0f, 0.0f, 0.0f, 0.0f },
                    { 0.0f, -1.0f, 0.0f, 0.0f },
                    { 0.0f, 0.0f, 1.0f, 0.0f },
                    { 0.0f, 0.0f, 0.0f, 1.f },
                }
            };

            ice::CameraData& render_data = params.frame.data().store<ice::CameraData>(camera->name);
            if (ortho != nullptr)
            {
                render_data.view = ice::lookat(
                    camera->position,
                    camera->position + camera->front,
                    { 0.f, 1.f, 0.f }
                );
                render_data.projection = clip * ice::orthographic(
                    ortho->left_right,
                    ortho->bottom_top,
                    ortho->near_far
                );
            }
            else if (persp != nullptr)
            {
                render_data.view = ice::lookat(
                    camera->position,
                    camera->position + camera->front,
                    { 0.f, 1.f, 0.f }
                );
                render_data.projection = clip * ice::perspective_fovx(
                    persp->field_of_view,
                    persp->aspect_ration,
                    persp->near_far.x,
                    persp->near_far.y
                );
            }
        }

        _ready.open();
        co_return;
    }

} // namespace ice
