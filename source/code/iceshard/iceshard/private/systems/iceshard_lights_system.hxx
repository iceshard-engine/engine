#pragma once
#include <core/allocator.hxx>

#include <iceshard/component/component_archetype_index.hxx>
#include <iceshard/component/component_archetype_iterator.hxx>
#include <iceshard/ecs/light.hxx>

#include <iceshard/engine.hxx>

namespace iceshard
{

    struct LightData
    {
        core::math::vec3 light_position[20];
        core::math::u32 light_count;
    };

    class IceshardIlluminationSystem final : public ComponentSystem
    {
    public:
        static constexpr auto SystemName = "isc.system.illumination"_sid;

        IceshardIlluminationSystem(
            core::allocator& alloc,
            iceshard::Engine& engine,
            iceshard::ecs::ArchetypeIndex& index,
            iceshard::renderer::RenderSystem& render_system
        ) noexcept;

        void update(iceshard::Engine& engine) noexcept override;

        auto update_buffers_task(
            LightData*
        ) noexcept -> cppcoro::task<>;

    private:
        iceshard::ecs::ArchetypeIndex& _archetype_index;
        iceshard::renderer::RenderSystem& _render_system;

        iceshard::ecs::ComponentQuery<
            iceshard::component::Entity*,
            iceshard::component::Light*
        > _component_query;

        // Render objects
        iceshard::renderer::api::Buffer _uniform_buffer;
    };

} // namespace iceshard
