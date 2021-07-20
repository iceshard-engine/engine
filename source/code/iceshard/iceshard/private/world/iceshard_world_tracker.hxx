#pragma once
#include "iceshard_world.hxx"
#include <ice/entity/entity_command_buffer.hxx>

namespace ice
{

    class Engine;
    class EngineRunner;

    class IceshardWorldTracker
    {
    public:
        IceshardWorldTracker(ice::Allocator& alloc) noexcept;
        ~IceshardWorldTracker() noexcept;

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
            ice::EngineRunner& runner,
            ice::Span<ice::EntityCommandBuffer::Command const> commands
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
