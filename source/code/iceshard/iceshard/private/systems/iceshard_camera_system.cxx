#include "iceshard_camera_system.hxx"

#include <input_system/message/mouse.hxx>
#include <input_system/message/keyboard.hxx>

#include <iceshard/frame.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_resources.hxx>

#include <core/math/matrix.hxx>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace
{
    template<typename To, typename From>
    auto math_cast(From const& src) noexcept -> To
    {
        static_assert(sizeof(From) == sizeof(To));

        To result;
        memcpy(std::addressof(result), std::addressof(src), sizeof(To));
        return result;
    }
}

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
        input::InputSystem& input_system,
        iceshard::renderer::RenderSystem& render_system
    ) noexcept
        : _archetype_index{ index }
        , _input_system{ input_system }
        , _render_system{ render_system }
        , _component_query{ alloc }
    {
        _uniform_buffer = _render_system.create_data_buffer(iceshard::renderer::api::BufferType::UniformBuffer, sizeof(CameraData));

        engine.add_task([](core::allocator& alloc, IceshardCameraSystem* self) noexcept -> cppcoro::task<>
            {
                using iceshard::renderer::RenderResource;
                using iceshard::renderer::RenderResourceType;

                core::pod::Array<RenderResource> resources{ alloc };
                core::pod::array::resize(resources, 1);
                resources[0].type = RenderResourceType::ResUniform;
                resources[0].handle.uniform.buffer = self->_uniform_buffer;
                resources[0].handle.uniform.offset = 0;
                resources[0].handle.uniform.range = sizeof(CameraData);
                resources[0].binding = 0;

                self->_render_system.update_resource_set(
                    "static-mesh.3d"_sid,
                    resources
                );

                co_return;
            }(alloc, this));
    }

    void IceshardCameraSystem::update(iceshard::Engine& engine) noexcept
    {
        using iceshard::component::Camera;
        using input::message::MouseMotion;
        using input::message::KeyboardKeyUp;
        using input::message::KeyboardKeyDown;
        using input::message::KeyboardModChanged;

        auto& frame = engine.current_frame();
        auto& frame_alloc = frame.frame_allocator();

        auto* camera_data = frame.new_frame_object<CameraData>(SystemName);

        float const camera_speed = 2.5f * frame.time_delta(); // adjust accordingly
        static glm::vec3 camera_up = { 0.0f, 1.0f, 0.0f };

        // Operations
        static float speed_mul = 1.0f;
        static bool camera_operations[6] = { false, false, false, false, false, false };

        // Get all asset models for rendering
        iceshard::ecs::for_each_entity(
            iceshard::ecs::query_index(_component_query, _archetype_index),
            [&](Camera* camera) noexcept
            {
                core::message::filter<KeyboardModChanged>(frame.messages(), [&](KeyboardModChanged const& ev) noexcept
                    {
                        if (ev.mod == input::KeyboardMod::ShiftLeft)
                        {
                            speed_mul = ev.pressed ? 4.0f : 1.0f;
                        }
                    });

                core::message::filter<KeyboardKeyDown>(frame.messages(), [&](KeyboardKeyDown const& ev) noexcept
                    {
                        using input::KeyboardKey;

                        switch (ev.key)
                        {
                        case KeyboardKey::Space:
                            camera_operations[0] = true;
                            break;
                        case KeyboardKey::KeyX:
                            camera_operations[1] = true;
                            break;
                        case KeyboardKey::KeyW:
                            camera_operations[2] = true;
                            break;
                        case KeyboardKey::KeyS:
                            camera_operations[3] = true;
                            break;
                        case KeyboardKey::KeyA:
                            camera_operations[4] = true;
                            break;
                        case KeyboardKey::KeyD:
                            camera_operations[5] = true;
                            break;
                        }
                    });

                core::message::filter<KeyboardKeyUp>(frame.messages(), [&](KeyboardKeyUp const& ev) noexcept
                    {
                        using input::KeyboardKey;

                        switch (ev.key)
                        {
                        case KeyboardKey::Space:
                            camera_operations[0] = false;
                            break;
                        case KeyboardKey::KeyX:
                            camera_operations[1] = false;
                            break;
                        case KeyboardKey::KeyW:
                            camera_operations[2] = false;
                            break;
                        case KeyboardKey::KeyS:
                            camera_operations[3] = false;
                            break;
                        case KeyboardKey::KeyA:
                            camera_operations[4] = false;
                            break;
                        case KeyboardKey::KeyD:
                            camera_operations[5] = false;
                            break;
                        }
                    });


                if (camera_operations[0])
                {
                    camera->position.y += camera_speed * speed_mul;
                }
                else if (camera_operations[1])
                {
                    camera->position.y -= camera_speed * speed_mul;
                }

                if (camera_operations[2])
                {
                    camera->position = math_cast<core::math::vec3>(
                        math_cast<glm::vec3>(camera->position) + math_cast<glm::vec3>(camera->front) * camera_speed * speed_mul
                        );
                }
                else if (camera_operations[3])
                {
                    camera->position = math_cast<core::math::vec3>(
                        math_cast<glm::vec3>(camera->position) - math_cast<glm::vec3>(camera->front) * camera_speed * speed_mul
                        );
                }

                if (camera_operations[4])
                {
                    camera->position = math_cast<core::math::vec3>(
                        math_cast<glm::vec3>(camera->position) -
                        glm::normalize(glm::cross(math_cast<glm::vec3>(camera->front), camera_up)) * camera_speed * speed_mul
                    );
                }
                else if (camera_operations[5])
                {
                    camera->position = math_cast<core::math::vec3>(
                        math_cast<glm::vec3>(camera->position) +
                        glm::normalize(glm::cross(math_cast<glm::vec3>(camera->front), camera_up)) * camera_speed * speed_mul
                    );
                }

                static bool first_mouse = true;
                core::message::filter<MouseMotion>(frame.messages(), [&](MouseMotion const& ev) noexcept
                    {
                        float xoffset = ev.pos.x;
                        float yoffset = -ev.pos.y;
                        if (ev.relative == false)
                        {
                            static float last_x = 0;
                            static float last_y = 0;

                            if (first_mouse) // initially set to true
                            {
                                last_x = ev.pos.x;
                                last_y = ev.pos.y;
                                first_mouse = false;
                            }

                            xoffset = ev.pos.x - last_x;
                            yoffset = last_y - ev.pos.y; // reversed since y-coordinates range from bottom to top
                            last_x = ev.pos.x;
                            last_y = ev.pos.y;
                        }


                        const float sensitivity = 0.2f;
                        xoffset *= sensitivity;
                        yoffset *= sensitivity;

                        camera->yaw += xoffset < 0 ? std::max(xoffset, -10.f) : std::min(xoffset, 10.f);
                        camera->pitch += yoffset < 0 ? std::max(yoffset, -10.f) : std::min(yoffset, 10.f);

                        if (camera->pitch > 89.0f)
                            camera->pitch = 89.0f;
                        if (camera->pitch < -89.0f)
                            camera->pitch = -89.0f;

                        glm::vec3 direction;
                        direction.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
                        direction.y = sin(glm::radians(camera->pitch));
                        direction.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
                        camera->front = math_cast<core::math::vec3>(glm::normalize(direction));
                    });

                camera_data->view = math_cast<core::math::mat4x4>(
                    glm::lookAt(
                        math_cast<glm::vec3>(camera->position),
                        math_cast<glm::vec3>(camera->position) + math_cast<glm::vec3>(camera->front),
                        camera_up
                    )
                );

                camera_data->projection = math_cast<core::math::mat4x4>(
                    glm::perspective(
                        camera->fovy,
                        16.0f / 9.0f,
                        0.1f, 100.0f
                    )
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

        engine.add_task(
            update_buffers_task(camera_data)
        );
    }

    auto IceshardCameraSystem::update_buffers_task(CameraData* camera_data) noexcept -> cppcoro::task<>
    {
        iceshard::renderer::api::DataView data_view;
        iceshard::renderer::api::render_api_instance->buffer_array_map_data(&_uniform_buffer, &data_view, 1);
        memcpy(data_view.data, camera_data, sizeof(CameraData));
        iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(&_uniform_buffer, 1);
        co_return;
    }

} // namespace iceshard
