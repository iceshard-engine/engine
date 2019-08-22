#include <iceshard/entity/entity_manager.hxx>

namespace iceshard::entity
{

    EntityManager::EntityManager(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _free_handles{ alloc }
        , _handles{ alloc }
    { }

} // namespace iceshard::entity
