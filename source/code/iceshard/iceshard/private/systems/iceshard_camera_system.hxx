#pragma once
#include <core/allocator.hxx>

#include <input_system/system.hxx>
#include <iceshard/component/component_archetype_index.hxx>
#include <iceshard/component/component_archetype_iterator.hxx>
#include <iceshard/ecs/camera.hxx>

#include <iceshard/engine.hxx>

namespace iceshard
{

    struct CameraData;

    class IceshardCameraSystem final : public ComponentSystem
    {
    public:
        static constexpr auto SystemName = "isc.system.camera"_sid;

        IceshardCameraSystem(
            core::allocator& alloc,
            iceshard::Engine& engine,
            iceshard::ecs::ArchetypeIndex& index,
            input::InputSystem& input_system,
            iceshard::renderer::RenderSystem& render_system
        ) noexcept;

        void update(iceshard::Engine& engine) noexcept override;

        auto update_buffers_task(CameraData* data) noexcept -> cppcoro::task<>;

    private:
        iceshard::ecs::ArchetypeIndex& _archetype_index;
        input::InputSystem& _input_system;
        iceshard::renderer::RenderSystem& _render_system;

        iceshard::ecs::ComponentQuery<
            iceshard::component::Camera*
        > _component_query;

        // Render objects
        iceshard::renderer::api::Buffer _uniform_buffer;
    };

} // namespace iceshard
