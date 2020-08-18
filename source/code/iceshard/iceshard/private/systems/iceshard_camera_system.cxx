#include "iceshard_camera_system.hxx"

#include <input_system/message/keyboard.hxx>

#include <iceshard/input/input_mouse.hxx>
#include <iceshard/input/input_keyboard.hxx>
#include <iceshard/input/input_controller.hxx>
#include <iceshard/input/device/input_device_queue.hxx>

#include <iceshard/frame.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_buffers.hxx>


#include <iceshard/math.hxx>

namespace iceshard
{

    struct CameraData
    {
        core::math::mat4x4 view;
        core::math::mat4x4 projection;
        core::math::mat4x4 clip;
    };

    IceshardCameraSystem::IceshardCameraSystem(
        core::allocator& alloc,
        iceshard::Engine& engine,
        iceshard::ecs::ArchetypeIndex& index,
        ::input::InputSystem& input_system,
        iceshard::renderer::RenderSystem& render_system
    ) noexcept
        : _archetype_index{ index }
        , _input_system{ input_system }
        , _render_system{ render_system }
        , _component_query{ alloc }
    {
        _uniform_buffer = renderer::create_buffer(renderer::api::BufferType::UniformBuffer, sizeof(CameraData));

        using iceshard::renderer::RenderResource;
        using iceshard::renderer::RenderResourceType;

        core::pod::Array<RenderResource> resources{ alloc };
        core::pod::array::resize(resources, 1);
        resources[0].type = RenderResourceType::ResUniform;
        resources[0].handle.uniform.buffer = _uniform_buffer;
        resources[0].handle.uniform.offset = 0;
        resources[0].handle.uniform.range = sizeof(CameraData);
        resources[0].binding = 0;

        _render_system.update_resource_set(
            "static-mesh.3d"_sid,
            resources
        );
    }

    void IceshardCameraSystem::update(iceshard::Frame& frame) noexcept
    {
        using namespace iceshard::input;
        using iceshard::component::Camera;

        auto& frame_alloc = frame.frame_allocator();

        auto* camera_data = frame.new_frame_object<CameraData>(SystemName);

        float const camera_speed = 2.5f * frame.elapsed_time(); // adjust accordingly
        static ism::vec3f camera_up = { 0.0f, 1.0f, 0.0f };

        // Operations
        static float speed_mul = 1.0f;
        static bool camera_operations[6] = { false, false, false, false, false, false };

        // Get all asset models for rendering
        iceshard::ecs::for_each_entity(
            iceshard::ecs::query_index(_component_query, _archetype_index),
            [&](Camera* camera) noexcept
            {
                using iceshard::input::DeviceInputType;

                const float sensitivity = 0.2f;

                bool speed_up = false;
                float move_y = 0.0f;
                float move_x = 0.0f;

                float xoffset = 0.f;
                float yoffset = 0.f;

                for (auto const& event : frame.input_events())
                {
                    switch (event.identifier)
                    {
                    case create_inputid(DeviceType::Controller, ControllerInput::LeftTrigger):
                        speed_up = event.value.axis.value_f32 > 0.5f;
                        break;
                    case create_inputid(DeviceType::Keyboard, KeyboardMod::ShiftLeft):
                        speed_up = event.value.button.state.pressed;
                        break;
                    case create_inputid(DeviceType::Keyboard, KeyboardKey::Space):
                        if (event.value.button.state.pressed)
                        {
                            camera->position.y += camera_speed * speed_mul;
                        }
                        break;
                    case create_inputid(DeviceType::Keyboard, KeyboardKey::KeyX):
                        if (event.value.button.state.pressed)
                        {
                            camera->position.y -= camera_speed * speed_mul;
                        }
                        break;
                    case create_inputid(DeviceType::Keyboard, KeyboardKey::KeyW):
                        move_y = event.value.button.state.pressed ? -1.0f : 0.0f;
                        break;
                    case create_inputid(DeviceType::Keyboard, KeyboardKey::KeyS):
                        move_y = event.value.button.state.pressed ? 1.0f : 0.0f;
                        break;
                    case create_inputid(DeviceType::Controller, ControllerInput::LeftAxisY):
                        if (event.value.axis.value_f32 >= 0.01f)
                        {
                            move_y = 1.0;
                        }
                        else if (event.value.axis.value_f32 <= -0.01f)
                        {
                            move_y = -1.0;
                        }
                        break;
                    case create_inputid(DeviceType::Keyboard, KeyboardKey::KeyA):
                        move_x = event.value.button.state.pressed ? 1.0f : 0.0f;
                        break;
                    case create_inputid(DeviceType::Keyboard, KeyboardKey::KeyD):
                        move_x = event.value.button.state.pressed ? -1.0f : 0.0f;
                        break;
                    case create_inputid(DeviceType::Controller, ControllerInput::LeftAxisX):
                        if (event.value.axis.value_f32 >= 0.01f)
                        {
                            move_x = -1.0;
                        }
                        else if (event.value.axis.value_f32 <= -0.01f)
                        {
                            move_x = 1.0;
                        }
                        break;
                    case create_inputid(DeviceType::Mouse, MouseInput::PositionXRelative):
                        xoffset = static_cast<float>(event.value.axis.value_i32) * camera_speed * 10.0;
                        break;
                    case create_inputid(DeviceType::Mouse, MouseInput::PositionYRelative):
                        yoffset = static_cast<float>(-event.value.axis.value_i32) * camera_speed * 10.0;
                        break;
                    case create_inputid(DeviceType::Controller, ControllerInput::RightAxisX):
                        xoffset = event.value.axis.value_f32 * camera_speed * 20.0;
                        break;
                    case create_inputid(DeviceType::Controller, ControllerInput::RightAxisY):
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

                camera->position = camera->position - camera->front * camera_speed * speed_mul * move_y;

                camera->position = camera->position + ism::normalize(
                    ism::cross(camera_up, camera->front)
                ) * camera_speed * speed_mul * move_x;


                camera->yaw += speed_mul * (xoffset < 0 ? std::max(xoffset, -10.f) : std::min(xoffset, 10.f));
                camera->pitch += speed_mul * (yoffset < 0 ? std::max(yoffset, -10.f) : std::min(yoffset, 10.f));

                if (camera->pitch > 89.0f)
                    camera->pitch = 89.0f;
                if (camera->pitch < -89.0f)
                    camera->pitch = -89.0f;

                ism::vec3f direction;
                direction.x = cos(ism::radians(camera->yaw)) * cos(ism::radians(camera->pitch));
                direction.y = sin(ism::radians(camera->pitch));
                direction.z = sin(ism::radians(camera->yaw)) * cos(ism::radians(camera->pitch));
                camera->front = ism::normalize(direction);

                camera_data->view = ism::lookat(
                    camera->position,
                    camera->position + camera->front,
                    camera_up
                );

                camera_data->projection = ism::perspective(
                    camera->fovy,
                    16.0f / 9.0f,
                    0.1f, 100.0f
                );

                camera_data->clip = core::math::mat4x4{
                    .v = {
                        { 1.0f, 0.0f, 0.0f, 0.0f },
                        { 0.0f, -1.0f, 0.0f, 0.0f },
                        { 0.0f, 0.0f, 0.5f, 0.5f },
                        { 0.0f, 0.0f, 0.0f, 0.1f },
                    }
                };
            }
        );

        frame.add_task(
            update_buffers_task(camera_data)
        );
    }

    auto IceshardCameraSystem::update_buffers_task(CameraData* camera_data) noexcept -> cppcoro::task<>
    {
        iceshard::renderer::api::DataView data_view;
        iceshard::renderer::api::render_module_api->buffer_array_map_data_func(&_uniform_buffer, &data_view, 1);
        memcpy(data_view.data, camera_data, sizeof(CameraData));
        iceshard::renderer::api::render_module_api->buffer_array_unmap_data_func(&_uniform_buffer, 1);
        co_return;
    }

} // namespace iceshard
