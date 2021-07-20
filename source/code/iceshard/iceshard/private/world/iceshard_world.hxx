#pragma once
#include <ice/world/world.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/entity/entity_command_buffer.hxx>

#include "iceshard_world_portal.hxx"

namespace ice
{

    class Engine;
    class EngineRunner;

    class IceshardWorld final : public ice::World
    {
    public:
        IceshardWorld(
            ice::Allocator& alloc,
            ice::EntityStorage* entity_storage
        ) noexcept;

        auto allocator() noexcept -> ice::Allocator& override;
        auto entity_storage() noexcept -> ice::EntityStorage& override;

        void add_trait(
            ice::StringID_Arg name,
            ice::WorldTrait* trait
        ) noexcept override;

        void remove_trait(
            ice::StringID_Arg name
        ) noexcept override;

        void activate(
            ice::Engine& engine,
            ice::EngineRunner& runner
        ) noexcept;

        void deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner
        ) noexcept;

        void update(
            ice::EngineRunner& runner,
            ice::Span<ice::EntityCommandBuffer::Command const> commands
        ) noexcept;

        auto traits() noexcept -> ice::pod::Array<ice::WorldTrait*>&;

    private:
        ice::Allocator& _allocator;
        ice::EntityStorage* _entity_storage;

        ice::pod::Array<ice::WorldTrait*> _traits;
        ice::pod::Hash<ice::IceshardWorldPortal*> _portals;
    };

} // namespace ice
