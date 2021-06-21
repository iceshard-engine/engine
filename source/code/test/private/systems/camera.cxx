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

#include <ice/render/render_swapchain.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_resource.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/input/input_controller.hxx>

#include <ice/math/lookat.hxx>
#include <ice/math/projection.hxx>

#include <ice/log.hxx>

namespace ice::trait
{

    struct CameraManager::RenderObjects
    {
        ice::vec2u extents;
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
        ice::Engine&,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        using namespace ice::gfx;
        using namespace ice::render;

        GfxDevice& gfx_device = runner.graphics_device();
        GfxResourceTracker& gfx_resources = gfx_device.resource_tracker();

        RenderSwapchain const& swapchain = gfx_device.swapchain();
        _render_objects->extents = swapchain.extent();
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

        track_resource(gfx_resources, "resourceset.camera"_sid, _render_objects->resource_set);
        track_resource(gfx_resources, "uniform_buffer.camera"_sid, _render_objects->uniform_data);

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
        ice::Engine&,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
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
        ice::WorldPortal& portal
    ) noexcept
    {
        static ice::vec3f camera_pos = { 0.f, 0.f, -1.f };
        static ice::vec3f camera_front = ice::vec3f{ 0.f, 0.f, 1.f };
        static ice::deg camera_yaw{ 0.f };
        static ice::deg camera_pitch{ 0.f };
        static float speed_mul = 1.0f;

        float const camera_speed = 5.f * (1.0f / 144.f); // adjust accordingly

        {
            using namespace ice::input;

            const float sensitivity = 0.2f;

            bool use_orto = false;
            bool speed_up = false;
            float move_y = 0.0f;
            float move_x = 0.0f;

            float xoffset = 0.f;
            float yoffset = 0.f;

            for (InputEvent const& event : frame.input_events())
            {
                switch (event.identifier)
                {
                case input_identifier(DeviceType::Controller, ControllerInput::LeftTrigger):
                    speed_up = event.value.axis.value_f32 > 0.5f;
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardMod::ShiftLeft, mod_identifier_base_value):
                    speed_up = event.value.button.state.pressed;
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardKey::Space, key_identifier_base_value):
                    if (event.value.button.state.pressed)
                    {
                        camera_pos.y += camera_speed * speed_mul;
                    }
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyX):
                    if (event.value.button.state.pressed)
                    {
                        camera_pos.y -= camera_speed * speed_mul;
                    }
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyW):
                    move_y = event.value.button.state.pressed ? -1.0f : 0.0f;
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyQ):
                    use_orto = !use_orto;
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyS):
                    move_y = event.value.button.state.pressed ? 1.0f : 0.0f;
                    break;
                case input_identifier(DeviceType::Controller, ControllerInput::LeftAxisY):
                    if (event.value.axis.value_f32 >= 0.01f)
                    {
                        move_y = 1.0;
                    }
                    else if (event.value.axis.value_f32 <= -0.01f)
                    {
                        move_y = -1.0;
                    }
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyA):
                    move_x = event.value.button.state.pressed ? 1.0f : 0.0f;
                    break;
                case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyD):
                    move_x = event.value.button.state.pressed ? -1.0f : 0.0f;
                    break;
                case input_identifier(DeviceType::Controller, ControllerInput::LeftAxisX):
                    if (event.value.axis.value_f32 >= 0.01f)
                    {
                        move_x = -1.0;
                    }
                    else if (event.value.axis.value_f32 <= -0.01f)
                    {
                        move_x = 1.0;
                    }
                    break;
                case input_identifier(DeviceType::Mouse, MouseInput::PositionXRelative):
                    xoffset = static_cast<float>(event.value.axis.value_i32) * camera_speed * 10.0;
                    break;
                case input_identifier(DeviceType::Mouse, MouseInput::PositionYRelative):
                    yoffset = static_cast<float>(-event.value.axis.value_i32) * camera_speed * 10.0;
                    break;
                case input_identifier(DeviceType::Controller, ControllerInput::RightAxisX):
                    xoffset = event.value.axis.value_f32 * camera_speed * 20.0;
                    break;
                case input_identifier(DeviceType::Controller, ControllerInput::RightAxisY):
                    yoffset = -event.value.axis.value_f32 * camera_speed * 20.0;
                    break;
                }
            }

            if (speed_up)
            {
                speed_mul = 4.0f;
            }
            else
            {
                speed_mul = 1.0f;
            }

            camera_pos = camera_pos - camera_front * camera_speed * speed_mul * move_y;

            camera_pos = camera_pos + ice::normalize(
                ice::cross({ 0.f, 1.f, 0.f }, camera_front)
            ) * camera_speed * speed_mul * move_x;


            camera_yaw.value += speed_mul * (xoffset < 0 ? std::max(xoffset, -10.f) : std::min(xoffset, 10.f));
            camera_pitch.value += speed_mul * (yoffset < 0 ? std::max(yoffset, -10.f) : std::min(yoffset, 10.f));

            if (camera_pitch.value > 89.0f)
                camera_pitch.value = 89.0f;
            if (camera_pitch.value < -89.0f)
                camera_pitch.value = -89.0f;

            ice::vec3f direction;
            direction.x = ice::cos(ice::radians(camera_yaw)) * ice::cos(ice::radians(camera_pitch));
            direction.y = ice::sin(ice::radians(camera_pitch));
            direction.z = ice::sin(ice::radians(camera_yaw)) * ice::cos(ice::radians(camera_pitch));
            camera_front = ice::normalize(direction);


            ice::gfx::GfxCameraUniform& camera_data = *frame.create_named_object<ice::gfx::GfxCameraUniform>("camera.uniform.data"_sid);

            if (!use_orto)
            {
                camera_data.view_matrix = ice::lookat(
                    { 0.f, 0.f, 0.f },
                    { 0.f, 0.f, -1.f },
                    { 0.f, 1.f, 0.f }
                );

                vec2f const extent{
                    (float)_render_objects->extents.x,
                    (float)_render_objects->extents.y
                };

                camera_data.projection_matrix = ice::orthographic(
                    vec2f{ 0.f, extent.x },
                    vec2f{ extent.y, 0.f },
                    vec2f{ 0.1f, 100.f }
                );

                camera_data.clip_matrix = {
                    .v = {
                        { 1.0f, 0.0f, 0.0f, 0.0f },
                        { 0.0f, -1.0f, 0.0f, 0.0f },
                        { 0.0f, 0.0f, 0.5f, 0.0f },
                        { 0.0f, 0.0f, 0.5f, 1.f },
                    }
                };
            }
            else
            {
                camera_data.view_matrix = ice::lookat(
                    camera_pos,
                    camera_pos + camera_front,
                    { 0.f, 1.f, 0.f }
                );
                camera_data.projection_matrix = ice::perspective(
                    ice::radians(ice::deg{ 70.f }),
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
            }

            ice::render::RenderDevice& device = runner.graphics_device().device();
            ice::render::BufferUpdateInfo camera_data_update[]{
                ice::render::BufferUpdateInfo{
                    .buffer = _render_objects->uniform_data,
                    .data = ice::data_view(camera_data)
                }
            };

            device.update_buffers(camera_data_update);
        }
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
