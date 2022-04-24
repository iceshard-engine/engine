#pragma once
#include <ice/world/world.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

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
            ice::ecs::EntityStorage* entity_storage
        ) noexcept;
        ~IceshardWorld() noexcept override;

        auto allocator() noexcept -> ice::Allocator& override;
        auto entity_storage() noexcept -> ice::ecs::EntityStorage& override;

        auto state_hint() const noexcept -> ice::WorldState override;
        void set_state(ice::WorldState state) noexcept;

        void add_trait(
            ice::StringID_Arg name,
            ice::WorldTrait* trait
        ) noexcept override;

        void add_owning_trait(
            ice::StringID_Arg name,
            ice::WorldTrait* trait
        ) noexcept;

        void remove_trait(
            ice::StringID_Arg name
        ) noexcept override;

        auto find_trait(
            ice::StringID_Arg name
        ) const noexcept -> ice::WorldTrait*;

        void activate(
            ice::Engine& engine,
            ice::EngineRunner& runner
        ) noexcept;

        void deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner
        ) noexcept;

        void update(
            ice::EngineRunner& runner
        ) noexcept;

        auto traits() noexcept -> ice::pod::Array<ice::WorldTrait*>&;

    private:
        ice::Allocator& _allocator;
        ice::ecs::EntityStorage* _entity_storage;
        ice::WorldState _state;

        ice::pod::Array<ice::WorldTrait*> _traits;
        ice::pod::Hash<ice::IceshardWorldPortal*> _portals;
    };

} // namespace ice
