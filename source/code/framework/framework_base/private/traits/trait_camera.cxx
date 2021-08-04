#include "trait_camera.hxx"
#include <ice/game_entity.hxx>
#include <ice/game_camera.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/archetype/archetype_query.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

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

    } // namespace detail

    IceWorldTrait_RenderCamera::IceWorldTrait_RenderCamera(ice::Allocator& alloc) noexcept
        : ice::gfx::GfxTrait{ }
        , _camera_buffers{ alloc }
    {
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
        runner.execute_task(
            task_update_cameras(frame, runner, portal),
            EngineContext::LogicFrame
        );
    }

    void IceWorldTrait_RenderCamera::gfx_update(
        ice::EngineFrame const& engine_frame,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        ice::Span<ice::TraitCameraData const> const& cameras = *engine_frame.named_object<ice::Span<ice::TraitCameraData const>>("ice.cameras.span"_sid);
        ice::u32 const camera_count = ice::size(cameras);

        ice::render::RenderDevice& device = gfx_device.device();

        for (ice::u32 idx = 0; idx < camera_count; ++idx)
        {
            if (_camera_buffers[idx] == ice::render::Buffer::Invalid)
            {
                _camera_buffers[idx] = device.create_buffer(
                    ice::render::BufferType::Uniform,
                    sizeof(ice::TraitCameraRenderData)
                );
            }

            ice::render::BufferUpdateInfo updates[]
            {
                ice::render::BufferUpdateInfo
                {
                    .buffer = _camera_buffers[idx],
                    .data = ice::data_view(cameras[idx].render_data),
                    .offset = 0
                }
            };

            device.update_buffers(updates);

            ice::gfx::track_resource(
                gfx_device.resource_tracker(),
                cameras[idx].camera_name,
                _camera_buffers[idx]
            );
        }
    }

    void IceWorldTrait_RenderCamera::gfx_setup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        for (ice::render::Buffer& buffer : _camera_buffers)
        {
            buffer = ice::render::Buffer::Invalid;
        }
    }

    void IceWorldTrait_RenderCamera::gfx_cleanup(
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxDevice& gfx_device
    ) noexcept
    {
        ice::render::RenderDevice& device = gfx_device.device();
        for (ice::render::Buffer const buffer : _camera_buffers)
        {
            if (buffer != ice::render::Buffer::Invalid)
            {
                device.destroy_buffer(buffer);
            }
        }
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

        ice::Span<ice::TraitCameraData> camera_span = frame.create_named_span<ice::TraitCameraData>("ice.cameras"_sid, camera_count);

        // Await work to be executed on a worker thread
        co_await runner.thread_pool();

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

                ice::TraitCameraData& camera_data = camera_span[cam_idx];
                camera_data.camera_name = cam.name;

                ice::TraitCameraRenderData& render_data = camera_data.render_data;
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
                    render_data.projection = ice::perspective_fovx(
                        persp->field_of_view,
                        persp->aspect_ration,
                        persp->near_far.x,
                        persp->near_far.y
                    );
                    render_data.clip = {
                        .v = {
                            { 1.0f, 0.0f, 0.0f, 0.0f },
                            { 0.0f, 1.0f, 0.0f, 0.0f },
                            { 0.0f, 0.0f, 1.0f, 0.0f },
                            { 0.0f, 0.0f, 0.0f, 1.0f },
                        }
                    };
                }

                cam_idx += 1;
            }
        );

        // TODO: Requires a thread-safe allocator implementation if we want to skip a revisit on the frame thread.
        co_await runner.schedule_current_frame();

        ice::u32 const current_buffer_count = ice::pod::array::size(_camera_buffers);
        ice::u32 const required_buffer_count = cam_idx;
        for (ice::u32 idx = current_buffer_count; idx < required_buffer_count; ++idx)
        {
            ice::pod::array::push_back(_camera_buffers, ice::render::Buffer::Invalid);
        }

        frame.create_named_object<ice::Span<ice::TraitCameraData>>("ice.cameras.span"_sid, camera_span.subspan(0, cam_idx));
        co_return;
    }

    auto create_trait_camera(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxTrait>
    {
        return ice::make_unique<ice::gfx::GfxTrait, ice::IceWorldTrait_RenderCamera>(alloc, alloc);
    }

} // namespace ice
