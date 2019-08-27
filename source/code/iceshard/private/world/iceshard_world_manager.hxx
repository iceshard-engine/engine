#pragma once
#include <iceshard/world/world_manager.hxx>
#include <iceshard/entity/entity_manager.hxx>

#include <core/allocator.hxx>
#include <core/pod/collections.hxx>

#include "iceshard_world.hxx"

namespace iceshard
{

    class IceshardWorldManager : public WorldManager
    {
    public:
        IceshardWorldManager(
            core::allocator& alloc,
            iceshard::ServiceProvider* engine_service_provider
        ) noexcept;

        ~IceshardWorldManager() noexcept override = default;

        auto get_world(core::cexpr::stringid_argument_type world_name) noexcept -> World* override;

        auto create_world(core::cexpr::stringid_argument_type world_name) noexcept -> World* override;

        void destroy_world(core::cexpr::stringid_argument_type world_name) noexcept override;

    private:
        core::allocator& _allocator;

        iceshard::ServiceProvider* const _engine_service_provider;

        core::pod::Hash<IceshardWorld*> _worlds;
    };

} // namespace iceshard::world
