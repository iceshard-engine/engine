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
            iceshard::ServiceProvider& engine_service_provider
        ) noexcept;

        ~IceshardWorldManager() noexcept override = default;

        auto get_world(core::stringid_arg_type world_name) noexcept -> World* override;

        auto create_world(core::stringid_arg_type world_name) noexcept -> World* override;

        void destroy_world(core::stringid_arg_type world_name) noexcept override;

        template<typename Fn>
        void foreach_world(Fn&& fn) noexcept
        {
            for (auto const& entry : _worlds)
            {
                std::forward<Fn>(fn)(*entry.value);
            }
        }

    private:
        core::allocator& _allocator;

        iceshard::ServiceProvider& _engine_service_provider;

        core::pod::Hash<IceshardWorld*> _worlds;
    };

} // namespace iceshard::world
