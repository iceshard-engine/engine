#include "iceshard_world.hxx"
#include "iceshard_world_service_provider.hxx"

#include <iceshard/engine.hxx>
#include <iceshard/component/component_system.hxx>
#include <iceshard/execution.hxx>

namespace iceshard
{

    IceshardWorld::IceshardWorld(
        core::allocator& alloc,
        core::stringid_arg_type world_name,
        iceshard::Entity world_entity,
        iceshard::ServiceProvider& engine_service_provider
    ) noexcept
        : World{ world_name, world_entity }
        , _allocator{ alloc }
        , _component_systems{ _allocator }
        , _service_provider{ nullptr, { _allocator } }
    {
        _service_provider = core::memory::make_unique<iceshard::IceshardWorldServiceProvider>(
            _allocator
            , _allocator
            , engine_service_provider
            , _component_systems
        );
    }

    IceshardWorld::~IceshardWorld() noexcept
    {
        for (auto const& entry : _component_systems)
        {
            _allocator.destroy(entry.value);
        }
    }

    void IceshardWorld::update(iceshard::ExecutionInstance& execution_instance) noexcept
    {
        for (auto const& entry : _component_systems)
        {
            entry.value->update(execution_instance.current_frame(), execution_instance.previous_frame());
        }
    }

    void IceshardWorld::add_component_system(
        core::stringid_arg_type component_name,
        ComponentSystemFactory factory,
        void* userdata
    ) noexcept
    {
        auto const component_system_hash = core::hash(component_name);
        IS_ASSERT(!core::pod::hash::has(_component_systems, component_system_hash), "Component with given name already exists!");

        core::pod::hash::set(_component_systems, component_system_hash, factory(_allocator, userdata));
    }

    auto IceshardWorld::service_provider() noexcept -> iceshard::ServiceProvider*
    {
        return _service_provider.get();
    }

} // namespace iceshard::world
