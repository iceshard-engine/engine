#include "iceshard_world.hxx"

namespace iceshard::world
{

    IceshardWorld::IceshardWorld(
        core::allocator& alloc,
        core::cexpr::stringid_argument_type world_name,
        iceshard::entity::entity_handle_type world_entity
    ) noexcept
        : World{ world_name, world_entity }
        , _allocator{ alloc }
    { }

    auto IceshardWorld::service_provider() noexcept -> component::ServiceProvider*
    {
        return nullptr;
    }

} // namespace iceshard::world
