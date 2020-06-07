#include "iceshard_lights_system.hxx"

#include <iceshard/frame.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/render_funcs.hxx>
#include <iceshard/renderer/render_buffers.hxx>

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
        _uniform_buffer = renderer::create_buffer(renderer::api::BufferType::UniformBuffer, sizeof(LightData));

        using iceshard::renderer::RenderResource;
        using iceshard::renderer::RenderResourceType;

        core::pod::Array<RenderResource> resources{ alloc };
        core::pod::array::resize(resources, 1);
        resources[0].type = RenderResourceType::ResUniform;
        resources[0].handle.uniform.buffer = _uniform_buffer;
        resources[0].handle.uniform.offset = 0;
        resources[0].handle.uniform.range = sizeof(LightData);
        resources[0].binding = 5;

        _render_system.update_resource_set(
            "static-mesh.3d"_sid,
            resources
        );
    }

    void IceshardIlluminationSystem::update(iceshard::Frame& frame) noexcept
    {
        using iceshard::component::Entity;
        using iceshard::component::Light;

        auto& frame_alloc = frame.frame_allocator();

        auto* light_data = frame.new_frame_object<LightData>(SystemName);

        // Get all asset models for rendering
        iceshard::ecs::for_each_entity(
            iceshard::ecs::query_index(_component_query, _archetype_index),
            [&light_data](Entity* e, Light* light) noexcept
            {
                light_data->point_lights[light_data->light_count].light_position = light->position;
                light_data->light_count += 1;
            }
        );

        frame.add_task(
            update_buffers_task(light_data)
        );
    }

    auto IceshardIlluminationSystem::update_buffers_task(LightData* light_data) noexcept -> cppcoro::task<>
    {
        iceshard::renderer::api::DataView data_view;
        iceshard::renderer::map_buffer(_uniform_buffer, data_view);

        memcpy(data_view.data, light_data, sizeof(LightData));
        iceshard::renderer::unmap_buffer(_uniform_buffer);
        co_return;
    }

} // namespace iceshard
