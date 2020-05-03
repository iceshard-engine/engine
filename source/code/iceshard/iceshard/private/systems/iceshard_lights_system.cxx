#include "iceshard_lights_system.hxx"

#include <iceshard/frame.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_resources.hxx>

namespace iceshard
{

    IceshardIlluminationSystem::IceshardIlluminationSystem(
        core::allocator& alloc,
        iceshard::Engine& engine,
        iceshard::ecs::ArchetypeIndex& index,
        iceshard::renderer::RenderSystem& render_system
    ) noexcept
        : _archetype_index{ index }
        , _render_system{ render_system }
        , _component_query{ alloc }
    {
        _uniform_buffer = _render_system.create_data_buffer(iceshard::renderer::api::BufferType::UniformBuffer, sizeof(LightData));

        engine.add_task([](core::allocator& alloc, IceshardIlluminationSystem* self) noexcept -> cppcoro::task<>
            {
                using iceshard::renderer::RenderResource;
                using iceshard::renderer::RenderResourceType;

                core::pod::Array<RenderResource> resources{ alloc };
                core::pod::array::resize(resources, 1);
                resources[0].type = RenderResourceType::ResUniform;
                resources[0].handle.uniform.buffer = self->_uniform_buffer;
                resources[0].handle.uniform.offset = 0;
                resources[0].handle.uniform.range = sizeof(LightData);
                resources[0].binding = 5;

                self->_render_system.update_resource_set(
                    "static-mesh.3d"_sid,
                    resources
                );

                co_return;
            }(alloc, this));
    }

    void IceshardIlluminationSystem::update(iceshard::Engine& engine) noexcept
    {
        using iceshard::component::Entity;
        using iceshard::component::Light;

        auto& frame = engine.current_frame();
        auto& frame_alloc = frame.frame_allocator();

        auto* light_data = frame.new_frame_object<LightData>(SystemName);

        // Get all asset models for rendering
        iceshard::ecs::for_each_entity(
            iceshard::ecs::query_index(_component_query, _archetype_index),
            [&light_data](Entity* e, Light* light) noexcept
            {
                light_data->light_position[light_data->light_count] = light->position;
                light_data->light_count += 1;
            }
        );

        engine.add_task(
            update_buffers_task(light_data)
        );
    }

    auto IceshardIlluminationSystem::update_buffers_task(LightData* light_data) noexcept -> cppcoro::task<>
    {
        iceshard::renderer::api::DataView data_view;
        iceshard::renderer::api::render_api_instance->buffer_array_map_data(&_uniform_buffer, &data_view, 1);
        memcpy(data_view.data, light_data, sizeof(LightData));
        iceshard::renderer::api::render_api_instance->buffer_array_unmap_data(&_uniform_buffer, 1);
        co_return;
    }

} // namespace iceshard
