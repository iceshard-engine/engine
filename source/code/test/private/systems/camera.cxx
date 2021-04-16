#include "camera.hxx"

#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_queue.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_resource.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>
#include <ice/gfx/gfx_camera.hxx>
#include <ice/gfx/gfx_subpass.hxx>

#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_resource.hxx>

#include <ice/math/lookat.hxx>
#include <ice/math/projection.hxx>

#include <ice/log.hxx>

namespace ice::trait
{

    struct CameraManager::RenderObjects
    {
        ice::render::Buffer uniform_data;
        ice::render::PipelineLayout pipeline_layout;
        ice::render::ResourceSet resource_set;
    };

    CameraManager::CameraManager(
        ice::Allocator& alloc,
        ice::Engine& engine
    ) noexcept
        : _allocator{ alloc }
        , _engine{ engine }
        , _render_objects{ _allocator.make<RenderObjects>() }
    {
    }

    CameraManager::~CameraManager() noexcept
    {
        _allocator.destroy(_render_objects);
    }

    void CameraManager::on_activate(
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        GfxDevice& gfx_device = runner.graphics_device();
        GfxResourceTracker& gfx_resources = gfx_device.resource_tracker();

        RenderDevice& device = gfx_device.device();

        _render_objects->uniform_data = device.create_buffer(
            BufferType::Uniform, sizeof(GfxCameraUniform)
        );

        _render_objects->pipeline_layout = find_resource<PipelineLayout>(gfx_resources, GfxSubpass_Primitives::ResName_PipelineLayout);

        ResourceSetLayout rs_layout = find_resource<ResourceSetLayout>(gfx_resources, GfxSubpass_Primitives::ResName_ResourceLayout);
        device.create_resourcesets(
            { &rs_layout, 1 },
            { &_render_objects->resource_set, 1 }
        );

        ResourceUpdateInfo resource_update_info[]{
            ResourceUpdateInfo{
                .uniform_buffer = {
                    .buffer = _render_objects->uniform_data,
                    .offset = 0,
                    .size = sizeof(GfxCameraUniform),
                }
            }
        };

        ResourceSetUpdateInfo set_update_info[]{
            ResourceSetUpdateInfo{
                .resource_set = _render_objects->resource_set,
                .resource_type = ResourceType::UniformBuffer,
                .binding_index = GfxSubpass_Primitives::ResConst_CameraUniformBinding,
                .resources = { resource_update_info }
            }
        };

        //GfxCameraUniform camera_data{ };

        //ice::render::BufferUpdateInfo camera_data_update[]{
        //    ice::render::BufferUpdateInfo{
        //        .buffer = _render_objects->uniform_data,
        //        .data = ice::data_view(camera_data)
        //    }
        //};

        //device.update_buffers(camera_data_update);

        device.update_resourceset(
            set_update_info
        );
    }

    void CameraManager::on_deactivate(
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        GfxDevice& gfx_device = runner.graphics_device();
        GfxResourceTracker& gfx_resources = gfx_device.resource_tracker();

        RenderDevice& device = gfx_device.device();

        device.destroy_buffer(_render_objects->uniform_data);
        device.destroy_resourcesets({ &_render_objects->resource_set, 1 });
    }

    void CameraManager::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
        static ice::vec4f camera_pos = { -2.f, 0.f, -2.f, 1.f };
        ice::vec3f look_at = { 0.f, 0.f, 0.f };

        static ice::deg angle = { 0.0f };
        angle.value += 1.f;

        //camera_pos.z -= 0.01;

        ice::vec4f temp_pos = ice::rotate(ice::radians(angle), { 0.f, 1.f, 0.f }) * camera_pos;
        ice::vec3f final_pos = { temp_pos.x / temp_pos.w, temp_pos.y / temp_pos.w, temp_pos.z / temp_pos.w };

        //camera_pos.z += 0.01f;

        ice::vec3f direction = ice::normalize(look_at - final_pos);

        ice::gfx::GfxCameraUniform& camera_data = *frame.create_named_object<ice::gfx::GfxCameraUniform>("camera.uniform.data"_sid);
        camera_data.view_matrix = ice::lookat(final_pos, final_pos + direction, { 0.f, 1.f, 0.f });
        camera_data.projection_matrix = ice::perspective(
            ice::radians(ice::deg{ 90.f }),
            16.f / 9.f,
            0.1f, 100.f
        );
        camera_data.clip_matrix = {
            .v = {
                { 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, -1.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.5f, 0.5f },
                { 0.0f, 0.0f, 0.0f, 0.1f },
            }
        };

        ice::render::RenderDevice& device = runner.graphics_device().device();
        ice::render::BufferUpdateInfo camera_data_update[]{
            ice::render::BufferUpdateInfo{
                .buffer = _render_objects->uniform_data,
                .data = ice::data_view(camera_data)
            }
        };

        device.update_buffers(camera_data_update);
    }

    void CameraManager::record_commands(
        ice::EngineFrame const& frame,
        ice::render::CommandBuffer cmds,
        ice::render::RenderCommands& api
    ) noexcept
    {
        api.bind_resource_set(
            cmds,
            _render_objects->pipeline_layout,
            _render_objects->resource_set,
            ice::gfx::GfxSubpass_Primitives::ResConst_CameraUniformSet
        );
    }

} // namespace ice::trait
