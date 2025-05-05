/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_camera.hxx"
#include <ice/game_entity.hxx>
#include <ice/game_camera.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_types_mappers.hxx>
#include <ice/world/world_trait_descriptor.hxx>

#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/ecs/ecs_query_tasks.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>

#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_shards.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>

#include <ice/math/lookat.hxx>
#include <ice/math/projection.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>

namespace ice
{

    static constexpr ice::ecs::ArchetypeDefinition<ice::Camera, ice::CameraOrtho> ArchDef_CameraOrtho{
        ice::ArchetypeName_OrtographicCamera
    };

    static constexpr ice::ecs::ArchetypeDefinition<ice::Camera, ice::CameraPerspective> ArchDef_CameraPersp{
        ice::ArchetypeName_PerspectiveCamera
    };

    void register_camera_archetypes(ice::ecs::ArchetypeIndex& archetypes) noexcept
    {
        archetypes.register_archetype(ArchDef_CameraOrtho);
        archetypes.register_archetype(ArchDef_CameraPersp);
    }

    auto IceWorldTrait_RenderCamera::trait_descriptor() noexcept -> ice::TraitDescriptor const&
    {
        static ice::TraitDescriptor const descriptor{
            .name = ice::TraitID_CameraManager,
            .fn_factory = ice::detail::default_trait_factory<IceWorldTrait_RenderCamera>,
            .fn_arch_register = register_camera_archetypes,
        };
        return descriptor;
    }

    IceWorldTrait_RenderCamera::IceWorldTrait_RenderCamera(
        ice::Allocator& alloc,
        ice::TraitContext& context
    ) noexcept
        : ice::Trait{ context }
        , _render_data{ alloc }
    {
        context.bind<&IceWorldTrait_RenderCamera::on_update>();
        context.bind<&IceWorldTrait_RenderCamera::on_gfx_update, gfx::GfxFrameUpdate const&, Graphics>(ice::gfx::ShardID_GfxFrameUpdate);
        context.bind<&IceWorldTrait_RenderCamera::on_gfx_shutdown, Render>(ice::gfx::ShardID_GfxShutdown);
        context.register_checkpoint("camera-data"_sid, _ready);
    }

    auto IceWorldTrait_RenderCamera::activate(
        ice::WorldStateParams const& params
    ) noexcept -> ice::Task<>
    {
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
        auto const& query_cameras = query<
            ice::ecs::Entity,
            ice::Camera const&,
            ice::CameraOrtho const*,
            ice::CameraPerspective const*
        >();

        //ice::u32 const camera_count = ice::ecs::query::entity_count(_query_cameras);
        ice::hashmap::reserve(_render_data, ice::ecs::query::entity_count(query_cameras));

        ice::ecs::QueryExecutionScope query_scope = co_await ice::ecs::await_query_on(
            query_cameras, params.thread.main
        );

        for (auto[entity, camera, ortho, persp] : ice::ecs::query::for_each_entity(query_scope))
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

            ice::TraitCameraData& data = ice::hashmap::get_or_set(_render_data, ice::hash(camera->name), { camera->name });
            ice::CameraData& camera_data = data.camera_data;
            if (ortho != nullptr)
            {
                camera_data.view = ice::lookat(
                    camera->position,
                    camera->position + camera->front,
                    { 0.f, 1.f, 0.f }
                );
                camera_data.projection = clip * ice::orthographic(
                    ortho->left_right,
                    ortho->bottom_top,
                    ortho->near_far
                );
            }
            else if (persp != nullptr)
            {
                camera_data.view = ice::lookat(
                    camera->position,
                    camera->position + camera->front,
                    { 0.f, 1.f, 0.f }
                );
                camera_data.projection = clip * ice::perspective_fovx(
                    persp->field_of_view,
                    persp->aspect_ration,
                    persp->near_far.x,
                    persp->near_far.y
                );
            }

            params.frame.data().store<ice::CameraData>(camera->name) = camera_data;
        }

        _ready.open();
        co_return;
    }

    auto IceWorldTrait_RenderCamera::on_gfx_update(
        ice::render::RenderDevice& device,
        ice::DataStorage& storage
    ) noexcept -> ice::Task<>
    {
        using namespace ice::render;

        for (ice::TraitCameraData& camera : ice::hashmap::values(_render_data))
        {
            if (camera.render_data == Buffer::Invalid)
            {
                camera.render_data = device.create_buffer(BufferType::Uniform, sizeof(ice::mat4));
                storage.read_or_store<ice::render::Buffer>(camera.name) = camera.render_data;
            }

            ice::mat4x4 const view_projection = camera.camera_data.projection * camera.camera_data.view;
            ice::render::BufferUpdateInfo const bui[]{
                {.buffer = camera.render_data, .data = ice::data_view(view_projection)}
            };

            device.update_buffers(bui);
        }

        co_return;
    }

    auto IceWorldTrait_RenderCamera::on_gfx_shutdown(
        ice::render::RenderDevice& device
    ) noexcept -> ice::Task<>
    {
        for (ice::TraitCameraData& data : ice::hashmap::values(_render_data))
        {
            device.destroy_buffer(data.render_data);
        }
        co_return;
    }

} // namespace ice
