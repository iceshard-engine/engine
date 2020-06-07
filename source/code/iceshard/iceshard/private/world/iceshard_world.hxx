#pragma once
#include <iceshard/world/world.hxx>
#include <core/pointer.hxx>

#include "iceshard_world_service_provider.hxx"

namespace iceshard
{

    class ExecutionInstance;

    class IceshardWorld : public World
    {
    public:
        IceshardWorld(
            core::allocator& alloc,
            core::stringid_arg_type world_name,
            iceshard::Entity world_entity,
            iceshard::ServiceProvider& engine_service_provider
        ) noexcept;

        ~IceshardWorld() noexcept override;

        void update(ExecutionInstance& engine) noexcept;

        void add_component_system(core::stringid_arg_type component_name, ComponentSystemFactory factory, void* userdata) noexcept override;

        //! \brief The worlds service provider.
        auto service_provider() noexcept -> iceshard::ServiceProvider* override;

    private:
        core::allocator& _allocator;

        core::pod::Hash<ComponentSystem*> _component_systems;

        core::memory::unique_pointer<iceshard::IceshardWorldServiceProvider> _service_provider;
    };

} // namespace iceshard::world