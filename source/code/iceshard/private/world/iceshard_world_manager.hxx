#pragma once
#include <iceshard/world/world_manager.hxx>
#include <core/allocator.hxx>

namespace iceshard::world
{

    class IceshardWorldManager : public WorldManager
    {
    public:
        IceshardWorldManager(core::allocator& alloc) noexcept;
        ~IceshardWorldManager() noexcept override = default;

    private:
        core::allocator& _allocator;
    };

} // namespace iceshard::world
