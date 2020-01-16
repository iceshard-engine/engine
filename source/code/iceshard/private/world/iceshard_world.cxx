#include "iceshard_world.hxx"
#include "iceshard_world_service_provider.hxx"

namespace iceshard
{

    IceshardWorld::IceshardWorld(
        core::allocator& alloc,
        core::stringid_arg_type world_name,
        iceshard::entity_handle_type world_entity,
        iceshard::ServiceProvider* engine_service_provider
    ) noexcept
        : World{ world_name, world_entity }
        , _allocator{ alloc }
        , _service_provider{ nullptr, { _allocator } }
    {
        _service_provider = core::memory::make_unique<iceshard::ServiceProvider, iceshard::IceshardWorldServiceProvider>(
            _allocator
            , _allocator
            , engine_service_provider
        );
    }

    auto IceshardWorld::service_provider() noexcept -> iceshard::ServiceProvider*
    {
        return _service_provider.get();
    }

} // namespace iceshard::world
