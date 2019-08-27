#pragma once
#include <iceshard/world/world_manager.hxx>
#include <iceshard/entity/entity_manager.hxx>

#include <core/allocator.hxx>
#include <core/pod/collections.hxx>

#include "iceshard_world.hxx"

namespace iceshard::world
{

    class IceshardWorldManager : public WorldManager
    {
    public:
        IceshardWorldManager(
            core::allocator& alloc,
            iceshard::entity::EntityManager* entity_manager
        ) noexcept;

        ~IceshardWorldManager() noexcept override = default;

        auto get_world(core::cexpr::stringid_argument_type world_name) noexcept -> World* override;

        auto create_world(core::cexpr::stringid_argument_type world_name) noexcept -> World* override;

        void destroy_world(core::cexpr::stringid_argument_type world_name) noexcept override;

    private:
        core::allocator& _allocator;

        iceshard::entity::EntityManager* const _entity_manager;

        core::pod::Hash<IceshardWorld*> _worlds;
    };

} // namespace iceshard::world
