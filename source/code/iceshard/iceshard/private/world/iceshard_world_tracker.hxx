#pragma once
#include "iceshard_world.hxx"

namespace ice
{

    class Engine;
    class EngineRunner;

    class IceshardWorldTracker
    {
    public:
        IceshardWorldTracker(ice::Allocator& alloc) noexcept;
        ~IceshardWorldTracker() noexcept;

        void set_managed_world(
            ice::IceshardWorld* world
        ) noexcept;

        void unset_manager_world(
            ice::IceshardWorld* world
        ) noexcept;

        void activate_world(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::IceshardWorld* world
        ) noexcept;

        void deactivate_world(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::IceshardWorld* world
        ) noexcept;

        void update_active_worlds(
            ice::EngineRunner& runner
        ) noexcept;

    private:
        ice::Allocator& _allocator;

        struct Entry
        {
            ice::IceshardWorld* world;
            ice::WorldState current_state;
        };

        ice::pod::Hash<Entry> _worlds;
    };

} // namespace ice
