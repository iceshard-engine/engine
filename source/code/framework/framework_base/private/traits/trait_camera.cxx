#include "trait_camera.hxx"
#include <ice/game_entity.hxx>
#include <ice/game_camera.hxx>

#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/archetype/archetype_query.hxx>

#include <ice/gfx/gfx_device.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_resource.hxx>
#include <ice/render/render_buffer.hxx>

#include <ice/math/lookat.hxx>
#include <ice/math/projection.hxx>

#include <ice/task_thread_pool.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        using QueryCamera = ice::ComponentQuery<ice::Entity, ice::Camera const&, ice::CameraOrtho const*, ice::CameraPerspective const*>;

        auto get_camera_query(ice::WorldPortal& portal) noexcept
        {
            return portal.storage().named_object<detail::QueryCamera>(
                "ice.trait.camera-query"_sid
            );
        }

        //auto get_camera_data(ice::DataStorage const& storage, ice::StringID_Arg name) noexcept -> ice::render::Buffer const*
        //{
        //    return storage.named_object<ice::render::Buffer>(name);
        //}

        //auto get_camera_data(ice::DataStorage& storage, ice::StringID_Arg name) noexcept -> ice::render::Buffer*
        //{
        //    ice::render::Buffer* data_buffer = storage.named_object<ice::render::Buffer>(name);
        //    if (data_buffer == nullptr)
        //    {
        //        data_buffer = storage.create_named_object<ice::render::Buffer>(name, ice::render::Buffer::Invalid);
        //    }
        //    return data_buffer;
        //}

    } // namespace detail

    IceWorldTrait_RenderCamera::IceWorldTrait_RenderCamera(ice::Allocator& alloc) noexcept
        : WorldTrait{ }
        , _camera_data{ alloc }
    {
        ice::pod::hash::reserve(_camera_data, 10);
    }

    void IceWorldTrait_RenderCamera::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().create_named_object<detail::QueryCamera>(
            "ice.trait.camera-query"_sid,
            portal.allocator(),
            portal.entity_storage().archetype_index()
        );
    }

    void IceWorldTrait_RenderCamera::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        auto task_destroy_uniform_buffer = [](ice::render::RenderDevice& device, ice::render::Buffer buffer) noexcept -> ice::Task<>
        {
            device.destroy_buffer(buffer);
            co_return;
        };

        for (auto const& entry : _camera_data)
        {
            runner.execute_task(
                task_destroy_uniform_buffer(runner.graphics_device().device(), entry.value->uniform_buffer),
                EngineContext::GraphicsFrame
            );
            portal.allocator().destroy(entry.value);
        }

        //detail::QueryCamera const* const camera_query = detail::get_camera_query(portal);
        //detail::QueryCamera::ResultByEntity camera_results = camera_query->result_by_entity(
        //    alloc,
        //    portal.entity_storage()
        //);

        portal.storage().destroy_named_object<detail::QueryCamera>(
            "ice.trait.camera-query"_sid
        );
    }

    void IceWorldTrait_RenderCamera::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.execute(task_update_cameras(frame, runner, portal));
    }

    auto IceWorldTrait_RenderCamera::task_update_cameras(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept -> ice::Task<>
    {
        ice::Allocator& alloc = frame.allocator();

        detail::QueryCamera const* const camera_query = detail::get_camera_query(portal);
        detail::QueryCamera::ResultByEntity camera_results = camera_query->result_by_entity(
            alloc,
            portal.entity_storage()
        );

        ice::u32 const camera_count = camera_results.entity_count();
        ice::pod::hash::reserve(_camera_data, ice::u32(ice::f32(camera_count) / 0.6));

        // Await work to be executed on a worker thread
        // TODO: Requires a thread-safe allocator implementation
        //co_await runner.thread_pool();

        ice::u32 cam_idx = 0;
        camera_results.for_each(
            [&, this](ice::Entity entity, ice::Camera const& cam, ice::CameraOrtho const* ortho, ice::CameraPerspective const* persp) noexcept
            {
                if (cam.name == ice::stringid_invalid)
                {
                    return;
                }

                ICE_ASSERT(
                    (ortho == nullptr && persp == nullptr) == false,
                    "Camera is not properly set-up, missing orthographics or perspective parameters."
                );
                ICE_ASSERT(
                    (ortho != nullptr && persp != nullptr) == false,
                    "Camera is not properly set-up, defining both orthographics and perspective parameters."
                );

                TraitCameraData* camera_data = ice::pod::hash::get(
                    _camera_data,
                    ice::hash(entity),
                    nullptr
                );

                if (camera_data == nullptr)
                {
                    camera_data = portal.allocator().make<TraitCameraData>(TraitCameraData{ .uniform_buffer = ice::render::Buffer::Invalid });
                    ice::pod::hash::set(_camera_data, ice::hash(entity), camera_data);
                }

                ice::TraitCameraRenderData& render_data = camera_data->render_data;
                if (ortho != nullptr)
                {
                    render_data.view = ice::lookat(
                        cam.position,
                        cam.position + cam.front,
                        { 0.f, 1.f, 0.f }
                    );
                    render_data.projection = ice::orthographic(
                        ortho->left_right,
                        ortho->top_bottom,
                        ortho->near_far
                    );
                    render_data.clip = {
                        .v = {
                            { 1.0f, 0.0f, 0.0f, 0.0f },
                            { 0.0f, -1.0f, 0.0f, 0.0f },
                            { 0.0f, 0.0f, 0.5f, 0.0f },
                            { 0.0f, 0.0f, 0.5f, 1.f },
                        }
                    };
                }
                else if (persp != nullptr)
                {
                    render_data.view = ice::lookat(
                        cam.position,
                        cam.position + cam.front,
                        { 0.f, 1.f, 0.f }
                    );
                    render_data.projection = ice::perspective(
                        persp->field_of_view,
                        persp->aspect_ration,
                        persp->near_far.x,
                        persp->near_far.y
                    );
                    render_data.clip = {
                        .v = {
                            { 1.0f, 0.0f, 0.0f, 0.0f },
                            { 0.0f, -1.0f, 0.0f, 0.0f },
                            { 0.0f, 0.0f, 0.5f, 0.5f },
                            { 0.0f, 0.0f, 0.0f, 0.1f },
                        }
                    };
                }

                if (camera_data->uniform_buffer != ice::render::Buffer::Invalid)
                {
                    frame.create_named_object<ice::render::Buffer>(cam.name, camera_data->uniform_buffer);
                }

                runner.execute_task(
                    task_update_camera_data(
                        runner.graphics_device().device(),
                        *camera_data
                    ),
                    EngineContext::GraphicsFrame
                );
            }
        );

        co_return;
        //co_await frame.schedule_frame_end();

        //ice::pod::array::clear(_camera_stages);
        //ice::pod::array::clear(_camera_stage_slots);
    }

    auto IceWorldTrait_RenderCamera::task_update_camera_data(
        ice::render::RenderDevice& device,
        ice::TraitCameraData& camera_data
    ) noexcept -> ice::Task<>
    {
        if (camera_data.uniform_buffer == ice::render::Buffer::Invalid)
        {
            camera_data.uniform_buffer = device.create_buffer(
                ice::render::BufferType::Uniform,
                sizeof(ice::TraitCameraRenderData)
            );
        }

        ice::render::BufferUpdateInfo updates[]
        {
            ice::render::BufferUpdateInfo
            {
                .buffer = camera_data.uniform_buffer,
                .data = ice::addressof(camera_data.render_data),
                .offset = 0
            }
        };

        device.update_buffers(updates);
        co_return;
    }

    //auto IceWorldTrait_RenderCamera::task_update_camera_data(
    //    ice::render::RenderDevice& device,
    //    ice::TraitCameraData& camera_data,
    //    ice::Camera const& camera,
    //    ice::CameraPerspective const& camera_persp
    //) noexcept -> ice::Task<>
    //{
    //    if (camera_data.uniform_buffer == ice::render::Buffer::Invalid)
    //    {
    //        camera_data.uniform_buffer = device.create_buffer(
    //            ice::render::BufferType::Uniform,
    //            sizeof(ice::mat4x4) * 2
    //        );
    //    }

    //    ice::render::BufferUpdateInfo updates[]
    //    {
    //        ice::render::BufferUpdateInfo
    //        {
    //            .buffer = camera_data.uniform_buffer,
    //            .data = ice::addressof(camera_data.render_data),
    //            .offset = 0
    //        }
    //    };

    //    device.update_buffers(updates);
    //    co_return;
    //}

    auto create_trait_camera(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>
    {
        return ice::make_unique<ice::WorldTrait, ice::IceWorldTrait_RenderCamera>(alloc, alloc);
    }

} // namespace ice
