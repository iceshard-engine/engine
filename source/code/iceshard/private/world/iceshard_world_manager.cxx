#include "iceshard_world_manager.hxx"

namespace iceshard::world
{

    IceshardWorldManager::IceshardWorldManager(core::allocator& alloc) noexcept
        : WorldManager{ }
        , _allocator{ alloc }
    { }

} // namespace iceshard::world
