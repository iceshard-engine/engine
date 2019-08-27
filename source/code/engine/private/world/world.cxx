#include <iceshard/world/world.hxx>

namespace iceshard::world
{

    World::World(core::cexpr::stringid_argument_type world_name, iceshard::entity::entity_handle_type world_entity) noexcept
        : _name{ world_name }
        , _entity{ world_entity }
    { }

    auto World::name() const noexcept -> core::cexpr::stringid_type
    {
        return _name;
    }

    auto World::entity() const noexcept -> iceshard::entity::entity_handle_type
    {
        return _entity;
    }

} // namespace iceshard::world
